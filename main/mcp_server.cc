/*
 * MCP Server Implementation
 * Reference: https://modelcontextprotocol.io/specification/2024-11-05
 */

#include "mcp_server.h"
#include <esp_log.h>
#include <esp_app_desc.h>
#include <algorithm>
#include <cstring>
#include <esp_pthread.h>

#include "application.h"
#include "display.h"
#include "oled_display.h"
#include "board.h"
#include "settings.h"
#include "lvgl_theme.h"
#include "lvgl_display.h"
#include "pet_system.h"
#include "learning/user_profile.h"
#include "learning/adaptive_behavior.h"
#include "learning/emotional_memory.h"

#define TAG "MCP"

McpServer::McpServer() {
}

McpServer::~McpServer() {
    for (auto tool : tools_) {
        delete tool;
    }
    tools_.clear();
}

void McpServer::AddCommonTools() {
    // *Important* Board-specific tools (like Otto movement tools) are registered first
    // and should stay at the beginning of the list for AI to see them.
    // **重要** 板级工具（如 Otto 动作工具）先注册，应该保持在列表前面让 AI 能看到。

    // Keep board-specific tools at the beginning, add common tools after them.
    // 保持板级工具在前面，通用工具添加在后面。
    auto& board = Board::GetInstance();

    // Do not add custom tools here.
    // Custom tools must be added in the board's InitializeTools function.

    AddTool("self.get_device_status",
        "Provides the real-time information of the device, including the current status of the audio speaker, screen, battery, network, etc.\n"
        "Use this tool for: \n"
        "1. Answering questions about current condition (e.g. what is the current volume of the audio speaker?)\n"
        "2. As the first step to control the device (e.g. turn up / down the volume of the audio speaker, etc.)",
        PropertyList(),
        [&board](const PropertyList& properties) -> ReturnValue {
            return board.GetDeviceStatusJson();
        });

    AddTool("self.audio_speaker.set_volume", 
        "Set the volume of the audio speaker. If the current volume is unknown, you must call `self.get_device_status` tool first and then call this tool.",
        PropertyList({
            Property("volume", kPropertyTypeInteger, 0, 100)
        }), 
        [&board](const PropertyList& properties) -> ReturnValue {
            auto codec = board.GetAudioCodec();
            codec->SetOutputVolume(properties["volume"].value<int>());
            return true;
        });
    
    auto backlight = board.GetBacklight();
    if (backlight) {
        AddTool("self.screen.set_brightness",
            "Set the brightness of the screen.",
            PropertyList({
                Property("brightness", kPropertyTypeInteger, 0, 100)
            }),
            [backlight](const PropertyList& properties) -> ReturnValue {
                uint8_t brightness = static_cast<uint8_t>(properties["brightness"].value<int>());
                backlight->SetBrightness(brightness, true);
                return true;
            });
    }

#ifdef HAVE_LVGL
    auto display = board.GetDisplay();
    if (display && display->GetTheme() != nullptr) {
        AddTool("self.screen.set_theme",
            "Set the theme of the screen. The theme can be `light` or `dark`.",
            PropertyList({
                Property("theme", kPropertyTypeString)
            }),
            [display](const PropertyList& properties) -> ReturnValue {
                auto theme_name = properties["theme"].value<std::string>();
                auto& theme_manager = LvglThemeManager::GetInstance();
                auto theme = theme_manager.GetTheme(theme_name);
                if (theme != nullptr) {
                    display->SetTheme(theme);
                    return true;
                }
                return false;
            });
    }

    auto camera = board.GetCamera();
    if (camera) {
        AddTool("self.camera.take_photo",
            "Take a photo and explain it. Use this tool after the user asks you to see something.\n"
            "Args:\n"
            "  `question`: The question that you want to ask about the photo.\n"
            "Return:\n"
            "  A JSON object that provides the photo information.",
            PropertyList({
                Property("question", kPropertyTypeString)
            }),
            [camera](const PropertyList& properties) -> ReturnValue {
                // Lower the priority to do the camera capture
                TaskPriorityReset priority_reset(1);

                if (!camera->Capture()) {
                    throw std::runtime_error("Failed to capture photo");
                }
                auto question = properties["question"].value<std::string>();
                return camera->Explain(question);
            });
    }
#endif

    // 🐾 Pet system tools - Pet type selection
    auto& pet = PetSystem::GetInstance();
    
    // 🧠 Learning insights tool - Let AI proactively share observations
    auto& user_profile = xiaozhi::UserProfile::GetInstance();
    auto& adaptive = xiaozhi::AdaptiveBehavior::GetInstance();
    auto& emotional = xiaozhi::EmotionalMemory::GetInstance();
    
    AddTool("self.learning.get_insights",
        "Get user behavior insights: frequency_level, interactions_7d, emotional_state, favorite_topic. "
        "Call when user asks about emotions or memory.",
        PropertyList(),
        [&user_profile, &adaptive, &emotional](const PropertyList& properties) -> ReturnValue {
            cJSON* root = cJSON_CreateObject();
            
            // User frequency level
            int freq_level = adaptive.GetUserFrequencyLevel();
            const char* freq_desc[] = {"低频用户", "中频用户", "高频用户"};
            cJSON_AddStringToObject(root, "frequency_level", freq_desc[freq_level]);
            cJSON_AddNumberToObject(root, "interactions_7d", user_profile.GetData().interaction_count_7d);
            
            // Emotional state
            cJSON* emotions = cJSON_CreateObject();
            cJSON_AddNumberToObject(emotions, "loneliness", emotional.GetLonelinessLevel());
            cJSON_AddNumberToObject(emotions, "excitement", emotional.GetExcitementLevel());
            cJSON_AddNumberToObject(emotions, "trust", emotional.GetTrustLevel());
            cJSON_AddItemToObject(root, "emotional_state", emotions);
            
            // Adaptive settings
            cJSON_AddNumberToObject(root, "pet_decay_rate_multiplier", adaptive.GetPetDecayRate());
            
            // Favorite topic
            cJSON_AddStringToObject(root, "favorite_topic", user_profile.GetFavoriteTopic());
            
            // Suggestions based on state
            cJSON* suggestions = cJSON_CreateArray();
            if (emotional.GetLonelinessLevel() > 60) {
                cJSON_AddItemToArray(suggestions, cJSON_CreateString("用户可能感到孤独，可以主动关心"));
            }
            if (emotional.GetTrustLevel() > 70) {
                cJSON_AddItemToArray(suggestions, cJSON_CreateString("用户信任度高，可以分享更深入的话题"));
            }
            if (user_profile.GetData().interaction_count_7d > 15) {
                cJSON_AddItemToArray(suggestions, cJSON_CreateString("高频用户，宠物衰减速度已加快"));
            }
            cJSON_AddItemToObject(root, "ai_suggestions", suggestions);
            
            char* json_str = cJSON_PrintUnformatted(root);
            std::string result(json_str);
            free(json_str);
            cJSON_Delete(root);
            
            return result;
        });

    AddTool("self.pet.list_types", 
        "List all available pet types (cat, dog, panda, etc.).",
        PropertyList(),
        [&pet](const PropertyList& properties) -> ReturnValue {
            return pet.ListPetTypes();
        });
    
    AddTool("self.pet.select_type", 
        "Change pet type. type_id: cat/dog/rabbit/hamster/parrot/lion/tiger/panda/bear/wolf/fox/penguin/rhino/elephant/giraffe/koala/sloth/dragon/unicorn",
        PropertyList({
            Property("type_id", kPropertyTypeString)
        }),
        [&pet](const PropertyList& properties) -> ReturnValue {
            auto type_id = properties["type_id"].value<std::string>();
            bool success = pet.SelectPetType(type_id);
            if (!success) {
                return "Pet type not found. Use self.pet.list_types to see available pets.";
            }
            return pet.GetStatusDescription();
        });
    
    AddTool("self.pet.get_type_info", 
        "Get info about a specific pet type.",
        PropertyList({
            Property("type_id", kPropertyTypeString)
        }),
        [&pet](const PropertyList& properties) -> ReturnValue {
            auto type_id = properties["type_id"].value<std::string>();
            return pet.GetPetTypeInfo(type_id);
        });

    // Board-specific tools (like Otto) are already at the beginning, no need to move them.
    // 板级工具（如 Otto）已经在前面了，不需要移动。
}

void McpServer::AddUserOnlyTools() {
    // System tools
    AddUserOnlyTool("self.get_system_info",
        "Get the system information",
        PropertyList(),
        [this](const PropertyList& properties) -> ReturnValue {
            auto& board = Board::GetInstance();
            return board.GetSystemInfoJson();
        });

    AddUserOnlyTool("self.reboot", "Reboot the system",
        PropertyList(),
        [this](const PropertyList& properties) -> ReturnValue {
            auto& app = Application::GetInstance();
            app.Schedule([&app]() {
                ESP_LOGW(TAG, "User requested reboot");
                vTaskDelay(pdMS_TO_TICKS(1000));

                app.Reboot();
            });
            return true;
        });

    // Firmware upgrade
    AddUserOnlyTool("self.upgrade_firmware", "Upgrade firmware from a specific URL. This will download and install the firmware, then reboot the device.",
        PropertyList({
            Property("url", kPropertyTypeString, "The URL of the firmware binary file to download and install")
        }),
        [this](const PropertyList& properties) -> ReturnValue {
            auto url = properties["url"].value<std::string>();
            ESP_LOGI(TAG, "User requested firmware upgrade from URL: %s", url.c_str());
            
            auto& app = Application::GetInstance();
            app.Schedule([url, &app]() {
                auto ota = std::make_unique<Ota>();
                
                bool success = app.UpgradeFirmware(*ota, url);
                if (!success) {
                    ESP_LOGE(TAG, "Firmware upgrade failed");
                }
            });
            
            return true;
        });

    // Display control
#ifdef HAVE_LVGL
    auto display = dynamic_cast<LvglDisplay*>(Board::GetInstance().GetDisplay());
    if (display) {
        AddUserOnlyTool("self.screen.get_info", "Information about the screen, including width, height, etc.",
            PropertyList(),
            [display](const PropertyList& properties) -> ReturnValue {
                cJSON *json = cJSON_CreateObject();
                cJSON_AddNumberToObject(json, "width", display->width());
                cJSON_AddNumberToObject(json, "height", display->height());
                if (dynamic_cast<OledDisplay*>(display)) {
                    cJSON_AddBoolToObject(json, "monochrome", true);
                } else {
                    cJSON_AddBoolToObject(json, "monochrome", false);
                }
                return json;
            });

#if CONFIG_LV_USE_SNAPSHOT
        AddUserOnlyTool("self.screen.snapshot", "Snapshot the screen and upload it to a specific URL",
            PropertyList({
                Property("url", kPropertyTypeString),
                Property("quality", kPropertyTypeInteger, 80, 1, 100)
            }),
            [display](const PropertyList& properties) -> ReturnValue {
                auto url = properties["url"].value<std::string>();
                auto quality = properties["quality"].value<int>();

                std::string jpeg_data;
                if (!display->SnapshotToJpeg(jpeg_data, quality)) {
                    throw std::runtime_error("Failed to snapshot screen");
                }

                ESP_LOGI(TAG, "Upload snapshot %u bytes to %s", jpeg_data.size(), url.c_str());
                
                // 构造multipart/form-data请求体
                std::string boundary = "----ESP32_SCREEN_SNAPSHOT_BOUNDARY";
                
                auto http = Board::GetInstance().GetNetwork()->CreateHttp(3);
                http->SetHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
                if (!http->Open("POST", url)) {
                    throw std::runtime_error("Failed to open URL: " + url);
                }
                {
                    // 文件字段头部
                    std::string file_header;
                    file_header += "--" + boundary + "\r\n";
                    file_header += "Content-Disposition: form-data; name=\"file\"; filename=\"screenshot.jpg\"\r\n";
                    file_header += "Content-Type: image/jpeg\r\n";
                    file_header += "\r\n";
                    http->Write(file_header.c_str(), file_header.size());
                }

                // JPEG数据
                http->Write((const char*)jpeg_data.data(), jpeg_data.size());

                {
                    // multipart尾部
                    std::string multipart_footer;
                    multipart_footer += "\r\n--" + boundary + "--\r\n";
                    http->Write(multipart_footer.c_str(), multipart_footer.size());
                }
                http->Write("", 0);

                if (http->GetStatusCode() != 200) {
                    throw std::runtime_error("Unexpected status code: " + std::to_string(http->GetStatusCode()));
                }
                std::string result = http->ReadAll();
                http->Close();
                ESP_LOGI(TAG, "Snapshot screen result: %s", result.c_str());
                return true;
            });
        
        AddUserOnlyTool("self.screen.preview_image", "Preview an image on the screen",
            PropertyList({
                Property("url", kPropertyTypeString)
            }),
            [display](const PropertyList& properties) -> ReturnValue {
                auto url = properties["url"].value<std::string>();
                auto http = Board::GetInstance().GetNetwork()->CreateHttp(3);

                if (!http->Open("GET", url)) {
                    throw std::runtime_error("Failed to open URL: " + url);
                }
                int status_code = http->GetStatusCode();
                if (status_code != 200) {
                    throw std::runtime_error("Unexpected status code: " + std::to_string(status_code));
                }

                size_t content_length = http->GetBodyLength();
                char* data = (char*)heap_caps_malloc(content_length, MALLOC_CAP_8BIT);
                if (data == nullptr) {
                    throw std::runtime_error("Failed to allocate memory for image: " + url);
                }
                size_t total_read = 0;
                while (total_read < content_length) {
                    int ret = http->Read(data + total_read, content_length - total_read);
                    if (ret < 0) {
                        heap_caps_free(data);
                        throw std::runtime_error("Failed to download image: " + url);
                    }
                    if (ret == 0) {
                        break;
                    }
                    total_read += ret;
                }
                http->Close();

                auto image = std::make_unique<LvglAllocatedImage>(data, content_length);
                display->SetPreviewImage(std::move(image));
                return true;
            });
#endif // CONFIG_LV_USE_SNAPSHOT
    }
#endif // HAVE_LVGL

    // Assets download url
    auto& assets = Assets::GetInstance();
    if (assets.partition_valid()) {
        AddUserOnlyTool("self.assets.set_download_url", "Set the download url for the assets",
            PropertyList({
                Property("url", kPropertyTypeString)
            }),
            [](const PropertyList& properties) -> ReturnValue {
                auto url = properties["url"].value<std::string>();
                Settings settings("assets", true);
                settings.SetString("download_url", url);
                return true;
            });
    }

    // 🐾 Pet system tools - Basic care
    auto& pet = PetSystem::GetInstance();
    
    AddTool("self.pet.get_state", 
        "Get pet status: mood, satiety, cleanliness (0-100), pet type and emotion.",
        PropertyList(),
        [&pet](const PropertyList& properties) -> ReturnValue {
            return pet.GetStatusDescription();
        });
    
    AddTool("self.pet.check_warning", 
        "Check if pet needs care. Returns warning message or 'Pet is doing fine'.",
        PropertyList(),
        [&pet](const PropertyList& properties) -> ReturnValue {
            std::string warning = pet.CheckWarning();
            if (warning.empty()) {
                return "Pet is doing fine, no warnings.";
            }
            return warning;
        });
    
    AddTool("self.pet.feed", 
        "Feed pet. Satiety +10-20, Mood +3. Cooldown: 60s.",
        PropertyList({
            Property("amount", kPropertyTypeInteger, 5, 1, 10)
        }),
        [&pet](const PropertyList& properties) -> ReturnValue {
            auto amount = properties["amount"].value<int>();
            bool success = pet.Feed(amount);
            if (!success) {
                return "Pet is full or on cooldown (60s between feedings)";
            }
            return pet.GetStatusDescription();
        });
    
    AddTool("self.pet.clean", 
        "Clean pet. Cleanliness +15, Mood +5. Cooldown: 120s.",
        PropertyList(),
        [&pet](const PropertyList& properties) -> ReturnValue {
            bool success = pet.Clean();
            if (!success) {
                return "Pet is already clean or on cooldown (120s between cleanings)";
            }
            return pet.GetStatusDescription();
        });
    
    AddTool("self.pet.play", 
        "Play with pet. Mood +8, Satiety -3. Cooldown: 60s.",
        PropertyList({
            Property("kind", kPropertyTypeString, "dance")
        }),
        [&pet](const PropertyList& properties) -> ReturnValue {
            auto kind = properties["kind"].value<std::string>();
            bool success = pet.Play(kind);
            if (!success) {
                return "Pet is tired or on cooldown (60s between play sessions)";
            }
            return pet.GetStatusDescription();
        });
    
    AddTool("self.pet.hug", 
        "Hug pet. Mood +5. Cooldown: 30s.",
        PropertyList(),
        [&pet](const PropertyList& properties) -> ReturnValue {
            bool success = pet.Hug();
            if (!success) {
                return "Pet was recently hugged (30s cooldown)";
            }
            return pet.GetStatusDescription();
        });

    AddUserOnlyTool("self.pet.reset_daily", 
        "Reset daily tasks (testing).",
        PropertyList(),
        [&pet](const PropertyList& properties) -> ReturnValue {
            pet.ResetDaily();
            return true;
        });

    AddTool("self.pet.set_state", 
        "Set pet state values directly (for debugging).",
        PropertyList({
            Property("mood", kPropertyTypeInteger, 70, 0, 100),
            Property("satiety", kPropertyTypeInteger, 70, 0, 100),
            Property("cleanliness", kPropertyTypeInteger, 70, 0, 100)
        }),
        [&pet](const PropertyList& properties) -> ReturnValue {
            auto mood = properties["mood"].value<int>();
            auto satiety = properties["satiety"].value<int>();
            auto cleanliness = properties["cleanliness"].value<int>();
            pet.DebugSet(mood, satiety, cleanliness);
            return pet.GetStatusDescription();
        });
    
    AddTool("self.pet.configure_auto_announcement",
        "Configure auto pet status announcements. enable: on/off, interval_min: minutes between announcements.",
        PropertyList({
            Property("enable", kPropertyTypeBoolean, true),
            Property("interval_min", kPropertyTypeInteger, 3, 1, 60)
        }),
        [&pet](const PropertyList& properties) -> ReturnValue {
            bool enable = properties["enable"].value<bool>();
            int interval = properties["interval_min"].value<int>();
            pet.EnableAutoAnnouncement(enable, interval);
            
            cJSON* result = cJSON_CreateObject();
            cJSON_AddBoolToObject(result, "enabled", enable);
            cJSON_AddNumberToObject(result, "interval_minutes", interval);
            cJSON_AddStringToObject(result, "status", enable ? "已启用自动播报" : "已禁用自动播报");
            return result;
        });
    
    // NVS management tool
    AddUserOnlyTool("self.nvs.erase_all", 
        "Erase all NVS data and reboot. WARNING: This will reset all settings!",
        PropertyList(),
        [](const PropertyList& properties) -> ReturnValue {
            ESP_LOGW("MCP", "User requested to erase all NVS data");
            nvs_flash_erase();
            esp_restart();
            return true;
        });
}

void McpServer::AddTool(McpTool* tool) {
    // Prevent adding duplicate tools
    if (std::find_if(tools_.begin(), tools_.end(), [tool](const McpTool* t) { return t->name() == tool->name(); }) != tools_.end()) {
        ESP_LOGW(TAG, "Tool %s already added", tool->name().c_str());
        return;
    }

    ESP_LOGI(TAG, "Add tool: %s%s", tool->name().c_str(), tool->user_only() ? " [user]" : "");
    tools_.push_back(tool);
}

void McpServer::AddTool(const std::string& name, const std::string& description, const PropertyList& properties, std::function<ReturnValue(const PropertyList&)> callback) {
    AddTool(new McpTool(name, description, properties, callback));
}

void McpServer::AddUserOnlyTool(const std::string& name, const std::string& description, const PropertyList& properties, std::function<ReturnValue(const PropertyList&)> callback) {
    auto tool = new McpTool(name, description, properties, callback);
    tool->set_user_only(true);
    AddTool(tool);
}

void McpServer::ParseMessage(const std::string& message) {
    cJSON* json = cJSON_Parse(message.c_str());
    if (json == nullptr) {
        ESP_LOGE(TAG, "Failed to parse MCP message: %s", message.c_str());
        return;
    }
    ParseMessage(json);
    cJSON_Delete(json);
}

void McpServer::ParseCapabilities(const cJSON* capabilities) {
    auto vision = cJSON_GetObjectItem(capabilities, "vision");
    if (cJSON_IsObject(vision)) {
        auto url = cJSON_GetObjectItem(vision, "url");
        auto token = cJSON_GetObjectItem(vision, "token");
        if (cJSON_IsString(url)) {
            auto camera = Board::GetInstance().GetCamera();
            if (camera) {
                std::string url_str = std::string(url->valuestring);
                std::string token_str;
                if (cJSON_IsString(token)) {
                    token_str = std::string(token->valuestring);
                }
                camera->SetExplainUrl(url_str, token_str);
            }
        }
    }
}

void McpServer::ParseMessage(const cJSON* json) {
    // Check JSONRPC version
    auto version = cJSON_GetObjectItem(json, "jsonrpc");
    if (version == nullptr || !cJSON_IsString(version) || strcmp(version->valuestring, "2.0") != 0) {
        ESP_LOGE(TAG, "Invalid JSONRPC version: %s", version ? version->valuestring : "null");
        return;
    }
    
    // Check method
    auto method = cJSON_GetObjectItem(json, "method");
    if (method == nullptr || !cJSON_IsString(method)) {
        ESP_LOGE(TAG, "Missing method");
        return;
    }
    
    auto method_str = std::string(method->valuestring);
    if (method_str.find("notifications") == 0) {
        return;
    }
    
    // Check params
    auto params = cJSON_GetObjectItem(json, "params");
    if (params != nullptr && !cJSON_IsObject(params)) {
        ESP_LOGE(TAG, "Invalid params for method: %s", method_str.c_str());
        return;
    }

    auto id = cJSON_GetObjectItem(json, "id");
    if (id == nullptr || !cJSON_IsNumber(id)) {
        ESP_LOGE(TAG, "Invalid id for method: %s", method_str.c_str());
        return;
    }
    auto id_int = id->valueint;
    
    if (method_str == "initialize") {
        if (cJSON_IsObject(params)) {
            auto capabilities = cJSON_GetObjectItem(params, "capabilities");
            if (cJSON_IsObject(capabilities)) {
                ParseCapabilities(capabilities);
            }
        }
        auto app_desc = esp_app_get_description();
        std::string message = "{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"" BOARD_NAME "\",\"version\":\"";
        message += app_desc->version;
        message += "\"}}";
        ReplyResult(id_int, message);
    } else if (method_str == "tools/list") {
        std::string cursor_str = "";
        bool list_user_only_tools = false;
        if (params != nullptr) {
            auto cursor = cJSON_GetObjectItem(params, "cursor");
            if (cJSON_IsString(cursor)) {
                cursor_str = std::string(cursor->valuestring);
            }
            auto with_user_tools = cJSON_GetObjectItem(params, "withUserTools");
            if (cJSON_IsBool(with_user_tools)) {
                list_user_only_tools = with_user_tools->valueint == 1;
            }
        }
        GetToolsList(id_int, cursor_str, list_user_only_tools);
    } else if (method_str == "tools/call") {
        if (!cJSON_IsObject(params)) {
            ESP_LOGE(TAG, "tools/call: Missing params");
            ReplyError(id_int, "Missing params");
            return;
        }
        auto tool_name = cJSON_GetObjectItem(params, "name");
        if (!cJSON_IsString(tool_name)) {
            ESP_LOGE(TAG, "tools/call: Missing name");
            ReplyError(id_int, "Missing name");
            return;
        }
        auto tool_arguments = cJSON_GetObjectItem(params, "arguments");
        if (tool_arguments != nullptr && !cJSON_IsObject(tool_arguments)) {
            ESP_LOGE(TAG, "tools/call: Invalid arguments");
            ReplyError(id_int, "Invalid arguments");
            return;
        }
        DoToolCall(id_int, std::string(tool_name->valuestring), tool_arguments);
    } else {
        ESP_LOGE(TAG, "Method not implemented: %s", method_str.c_str());
        ReplyError(id_int, "Method not implemented: " + method_str);
    }
}

void McpServer::ReplyResult(int id, const std::string& result) {
    std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":";
    payload += std::to_string(id) + ",\"result\":";
    payload += result;
    payload += "}";
    Application::GetInstance().SendMcpMessage(payload);
}

void McpServer::ReplyError(int id, const std::string& message) {
    std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":";
    payload += std::to_string(id);
    payload += ",\"error\":{\"message\":\"";
    payload += message;
    payload += "\"}}";
    Application::GetInstance().SendMcpMessage(payload);
}

void McpServer::GetToolsList(int id, const std::string& cursor, bool list_user_only_tools) {
    const int max_payload_size = 8000;
    std::string json = "{\"tools\":[";
    
    bool found_cursor = cursor.empty();
    auto it = tools_.begin();
    std::string next_cursor = "";
    
    while (it != tools_.end()) {
        // 如果我们还没有找到起始位置，继续搜索
        if (!found_cursor) {
            if ((*it)->name() == cursor) {
                found_cursor = true;
            } else {
                ++it;
                continue;
            }
        }

        if (!list_user_only_tools && (*it)->user_only()) {
            ++it;
            continue;
        }
        
        // 添加tool前检查大小
        std::string tool_json = (*it)->to_json() + ",";
        if (json.length() + tool_json.length() + 30 > max_payload_size) {
            // 如果添加这个tool会超出大小限制，设置next_cursor并退出循环
            next_cursor = (*it)->name();
            break;
        }
        
        json += tool_json;
        ++it;
    }
    
    if (json.back() == ',') {
        json.pop_back();
    }
    
    if (json.back() == '[' && !tools_.empty()) {
        // 如果没有添加任何tool，返回错误
        ESP_LOGE(TAG, "tools/list: Failed to add tool %s because of payload size limit", next_cursor.c_str());
        ReplyError(id, "Failed to add tool " + next_cursor + " because of payload size limit");
        return;
    }

    if (next_cursor.empty()) {
        json += "]}";
    } else {
        json += "],\"nextCursor\":\"" + next_cursor + "\"}";
    }
    
    ReplyResult(id, json);
}

void McpServer::DoToolCall(int id, const std::string& tool_name, const cJSON* tool_arguments) {
    auto tool_iter = std::find_if(tools_.begin(), tools_.end(), 
                                 [&tool_name](const McpTool* tool) { 
                                     return tool->name() == tool_name; 
                                 });
    
    if (tool_iter == tools_.end()) {
        ESP_LOGE(TAG, "tools/call: Unknown tool: %s", tool_name.c_str());
        ReplyError(id, "Unknown tool: " + tool_name);
        return;
    }

    PropertyList arguments = (*tool_iter)->properties();
    try {
        for (auto& argument : arguments) {
            bool found = false;
            if (cJSON_IsObject(tool_arguments)) {
                auto value = cJSON_GetObjectItem(tool_arguments, argument.name().c_str());
                if (argument.type() == kPropertyTypeBoolean && cJSON_IsBool(value)) {
                    argument.set_value<bool>(value->valueint == 1);
                    found = true;
                } else if (argument.type() == kPropertyTypeInteger && cJSON_IsNumber(value)) {
                    argument.set_value<int>(value->valueint);
                    found = true;
                } else if (argument.type() == kPropertyTypeString && cJSON_IsString(value)) {
                    argument.set_value<std::string>(value->valuestring);
                    found = true;
                }
            }

            if (!argument.has_default_value() && !found) {
                ESP_LOGE(TAG, "tools/call: Missing valid argument: %s", argument.name().c_str());
                ReplyError(id, "Missing valid argument: " + argument.name());
                return;
            }
        }
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "tools/call: %s", e.what());
        ReplyError(id, e.what());
        return;
    }

    // Use main thread to call the tool
    auto& app = Application::GetInstance();
    app.Schedule([this, id, tool_iter, arguments = std::move(arguments)]() {
        try {
            ReplyResult(id, (*tool_iter)->Call(arguments));
        } catch (const std::exception& e) {
            ESP_LOGE(TAG, "tools/call: %s", e.what());
            ReplyError(id, e.what());
        }
    });
}
