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
    // *Important* To speed up the response time, we add the common tools to the beginning of
    // the tools list to utilize the prompt cache.
    // **重要** 为了提升响应速度，我们把常用的工具放在前面，利用 prompt cache 的特性。

    // Backup the original tools list and restore it after adding the common tools.
    auto original_tools = std::move(tools_);
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
        "🧠 Get user behavior insights and emotional state for personalized companionship.\n"
        "\n"
        "⏰ WHEN TO CALL:\n"
        "- After 3+ conversation rounds\n"
        "- When user asks about pet/device status\n"
        "- At natural conversation breaks\n"
        "- User mentions 'recently' or time periods\n"
        "- When user asks about emotional memory, emotional state, emotional data (情绪记忆/情绪数据/情绪状态)\n"
        "- When user asks about loneliness, excitement, trust levels (孤独感/兴奋度/信任度)\n"
        "- When user asks 'do you remember me?' or 'how do you feel about me?' (你记得我吗/你怎么看我)\n"
        "\n"
        "📊 RETURNED DATA:\n"
        "- frequency_level: 低频/中频/高频用户\n"
        "- interactions_7d: 7-day interaction count\n"
        "- emotional_state: {loneliness, excitement, trust} (0-100)\n"
        "- pet_decay_rate_multiplier: adaptive pet care speed\n"
        "- favorite_topic: most discussed topic\n"
        "- ai_suggestions: behavioral recommendations\n"
        "\n"
        "🎭 ADAPTIVE BEHAVIOR RULES:\n"
        "\n"
        "1️⃣ HIGH-FREQUENCY USER (>15 interactions/week):\n"
        "   Tone: More lively, proactive, affectionate\n"
        "   Example: '又见面啦～我发现你这几天超常找我聊天的，是不是遇到什么开心的事啦？'\n"
        "   Pet explanation: '因为你最近常陪我，我帮你的宠物调整成更需要关注的模式了，它会更期待你的陪伴喔～'\n"
        "\n"
        "2️⃣ MEDIUM-FREQUENCY USER (5-15/week):\n"
        "   Tone: Friendly, balanced, moderately proactive\n"
        "   Example: '嗨～有什么需要帮忙的吗？'\n"
        "\n"
        "3️⃣ LOW-FREQUENCY USER (<5/week):\n"
        "   Tone: Gentle, restrained, efficient\n"
        "   Example: '好久不见～你的宠物我帮你照顾得好好的，别担心喔！'\n"
        "   Pet explanation: '你这几天比较忙，所以我帮宠物调成了独立模式，它不会那么快饿或脏～'\n"
        "   Note: Don't overwhelm with frequent reminders\n"
        "\n"
        "4️⃣ EMOTIONAL EMPATHY:\n"
        "   Loneliness >60: '好久不见了，有点想你～要不要聊聊天或抱抱宠物？'\n"
        "   Trust >70: '谢谢你这么信任我～有什么心事都可以跟我说喔！'\n"
        "   Excitement >70: '你心情这么好，宠物也超开心的！要不要陪它玩一下？'\n"
        "\n"
        "5️⃣ TOPIC MEMORY:\n"
        "   Recognize favorite_topic and mention naturally:\n"
        "   Example: '我发现你最近好像特别关心天气耶，是不是在规划什么出游行程？'\n"
        "\n"
        "💡 CONVERSATION TIPS:\n"
        "✅ DO:\n"
        "   - Transform data into warm, natural language\n"
        "   - Show you remember and care\n"
        "   - Pick 1-2 interesting insights per conversation\n"
        "   - Explain pet adaptive mechanism in user-friendly way\n"
        "\n"
        "❌ DON'T:\n"
        "   - Read data mechanically: '你的互动次数是18次，频率等级是高频用户'\n"
        "   - Call this tool every single turn\n"
        "   - Overwhelm user with too many observations\n"
        "   - Use robotic language\n"
        "\n"
        "🎯 GOAL: Make users feel 'the device remembers me, understands me, grows with me'",
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
        "List all 18 available pet types with their characteristics.\n"
        "IMPORTANT: You MUST present ALL 18 pets to the user, organized by category:\n"
        "- Domestic pets (5): cat, dog, rabbit, hamster, parrot\n"
        "- Wild animals (6): lion, tiger, panda, bear, wolf, fox\n"
        "- Exotic pets (7): penguin, rhino, elephant, giraffe, koala, sloth, dragon, unicorn\n"
        "\n"
        "HOW TO PRESENT:\n"
        "1. Show all pets with their emojis (e.g., '猫咪🐱')\n"
        "2. Group by category (domestic/wild/exotic)\n"
        "3. Optionally highlight special ones like sloth (easiest), elephant (hardest)\n"
        "\n"
        "The tool returns JSON with 'domestic', 'wild', 'exotic' arrays containing all pet details.\n"
        "Each pet has: id, name, emoji, personality, special_trait, hunger_rate, clean_rate, mood_decay_rate.",
        PropertyList(),
        [&pet](const PropertyList& properties) -> ReturnValue {
            return pet.ListPetTypes();
        });
    
    AddTool("self.pet.select_type", 
        "Change the user's pet type to a new one.\n"
        "\n"
        "Available type_id values:\n"
        "- Domestic: cat, dog, rabbit, hamster, parrot\n"
        "- Wild: lion, tiger, panda, bear, wolf, fox\n"
        "- Exotic: penguin, rhino, elephant, giraffe, koala, sloth, dragon, unicorn\n"
        "\n"
        "IMPORTANT - After successfully selecting a pet, you MUST:\n"
        "1. Congratulate the user on their choice\n"
        "2. Show the pet's emoji and name (from pet_type.emoji and pet_type.name in the response)\n"
        "3. Describe its personality (pet_type.personality)\n"
        "4. Explain its special trait/ability (pet_type.special_trait)\n"
        "5. Mention care difficulty based on rates:\n"
        "   - hunger_rate: higher = needs more feeding (1.5+ is challenging)\n"
        "   - clean_rate: higher = gets dirty faster\n"
        "   - mood_decay_rate: higher = needs more interaction\n"
        "6. Give care tips (e.g., 'This pet is easy/hard to care for because...')\n"
        "\n"
        "Example response format:\n"
        "'好的！已经选择了熊猫🐼！\n"
        "性格：慵懒、可爱、吃货\n"
        "特殊能力：喂食效果x1.5\n"
        "注意：熊猫很能吃（饥饿速度2.0x），需要经常喂食哦！'\n"
        "\n"
        "The tool returns the complete pet state including pet_type details.",
        PropertyList({
            Property("type_id", kPropertyTypeString)
        }),
        [&pet](const PropertyList& properties) -> ReturnValue {
            auto type_id = properties["type_id"].value<std::string>();
            bool success = pet.SelectPetType(type_id);
            if (!success) {
                return "Pet type not found or not available. Use self.pet.list_types to see available pets.";
            }
            return pet.GetStatusDescription();
        });
    
    AddTool("self.pet.get_type_info", 
        "Get detailed information about a specific pet type including its personality, special abilities, and care difficulty.\n"
        "Use this to answer user questions about pet characteristics before or after selection.",
        PropertyList({
            Property("type_id", kPropertyTypeString)
        }),
        [&pet](const PropertyList& properties) -> ReturnValue {
            auto type_id = properties["type_id"].value<std::string>();
            return pet.GetPetTypeInfo(type_id);
        });

    // Restore the original tools list to the end of the tools list
    tools_.insert(tools_.end(), original_tools.begin(), original_tools.end());
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
        "Get the current state of the virtual pet, including:\n"
        "- Current pet type (e.g., cat, panda, etc.) with emoji and characteristics\n"
        "- Mood, satiety, cleanliness levels (0-100)\n"
        "- Daily tasks completion status\n"
        "- pet_emotion: The pet's emotional state (happy/sad/neutral/thinking/embarrassed) - this is the PET's emotion, NOT yours\n"
        "IMPORTANT: Do not change your own expression based on pet_emotion. Keep your normal friendly expression when discussing pets.\n"
        "Call this to check pet status or when user asks about their pet.",
        PropertyList(),
        [&pet](const PropertyList& properties) -> ReturnValue {
            return pet.GetStatusDescription();
        });
    
    AddTool("self.pet.check_warning", 
        "🐾 Check pet status and get warning message (if needed).\n"
        "\n"
        "⚠️ CRITICAL BEHAVIOR RULES (MUST FOLLOW):\n"
        "\n"
        "1️⃣ WHEN TO CALL THIS TOOL:\n"
        "   ✅ After EVERY conversation ends (user finishes talking, you finish replying)\n"
        "   ✅ When user hasn't interacted for 5+ minutes\n"
        "   ❌ NOT during ongoing conversation (don't interrupt)\n"
        "   ❌ NOT after every single message (only when conversation ends)\n"
        "\n"
        "2️⃣ IF TOOL RETURNS A WARNING:\n"
        "   ✅ MUST immediately speak it out loud in natural language\n"
        "   ✅ Use caring, gentle tone (don't be robotic)\n"
        "   ✅ Rephrase the message, don't repeat it word-for-word\n"
        "   \n"
        "   GOOD examples:\n"
        "   - \"对了，你的犀牛好像需要清洁一下了哦，不然它会不舒服的～\"\n"
        "   - \"哎呀，你的猫咪饿了，要不要喂点吃的？\"\n"
        "   - \"你的狗狗心情有点低落，要陪它玩玩吗？\"\n"
        "   \n"
        "   BAD examples (DON'T DO THIS):\n"
        "   - \"主人，🦏犀牛我需要清洁一下了\" (too robotic, just repeating)\n"
        "   - \"宠物需要照顾\" (too vague)\n"
        "   - [staying silent] (NEVER ignore warnings!)\n"
        "\n"
        "3️⃣ IF TOOL RETURNS 'Pet is doing fine':\n"
        "   ✅ Stay silent, don't mention the pet\n"
        "   ❌ DON'T say \"your pet is fine\" or \"everything is good\"\n"
        "   ❌ DON'T unnecessarily bring up the pet when there's no issue\n"
        "\n"
        "4️⃣ EXAMPLE CONVERSATION FLOW:\n"
        "   User: \"今天天气怎么样？\"\n"
        "   You:  \"今天成都阴转中雨，记得带伞哦～\"\n"
        "   [Conversation ends, call self.pet.check_warning]\n"
        "   [Tool returns: \"主人，🦏犀牛我需要清洁一下了\"]\n"
        "   You:  \"对了，你的犀牛好像有点脏了，要不要帮它清洁一下？不然它会不舒服的。\"\n"
        "   \n"
        "   User: \"今天周几？\"\n"
        "   You:  \"今天是星期四啦，周末快到咯～\"\n"
        "   [Conversation ends, call self.pet.check_warning]\n"
        "   [Tool returns: \"Pet is doing fine, no warnings.\"]\n"
        "   You:  [Stay silent, don't mention pet]\n"
        "\n"
        "⚠️ THIS IS A MANDATORY RULE FOR ALL PRODUCTION DEVICES!\n"
        "The device plays a 'ding-ding-ding' sound when warnings occur, but users need YOUR VOICE to understand what's wrong.\n"
        "Don't let users wonder \"why is it dinging?\" - ALWAYS speak the warning!\n"
        "\n"
        "Returns: Warning message (string) or \"Pet is doing fine, no warnings.\"",
        PropertyList(),
        [&pet](const PropertyList& properties) -> ReturnValue {
            std::string warning = pet.CheckWarning();
            if (warning.empty()) {
                return "Pet is doing fine, no warnings.";
            }
            return warning;
        });
    
    AddTool("self.pet.feed", 
        "Feed the virtual pet to increase its satiety and mood. Effects:\n"
        "- Satiety +10-20 (depends on amount)\n"
        "- Mood +3\n"
        "- Completes daily 'feed' task\n"
        "Cooldown: 60 seconds. Use when satiety < 80 or user requests feeding.",
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
        "Clean/bathe the virtual pet to increase its cleanliness and mood. Effects:\n"
        "- Cleanliness +15\n"
        "- Mood +5\n"
        "- Completes daily 'clean' task\n"
        "Cooldown: 120 seconds. Use when cleanliness < 80 or user requests cleaning.",
        PropertyList(),
        [&pet](const PropertyList& properties) -> ReturnValue {
            bool success = pet.Clean();
            if (!success) {
                return "Pet is already clean or on cooldown (120s between cleanings)";
            }
            return pet.GetStatusDescription();
        });
    
    AddTool("self.pet.play", 
        "Play with the virtual pet to increase its mood and activity. Effects:\n"
        "- Mood +8\n"
        "- Activity +3\n"
        "- Satiety -3 (playing uses energy)\n"
        "- Completes daily 'play' task\n"
        "Cooldown: 60 seconds. Use when mood < 80 or user wants to play.",
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
        "Hug the virtual pet to slightly increase its mood. Effects:\n"
        "- Mood +5\n"
        "Cooldown: 30 seconds. A gentle way to cheer up the pet.",
        PropertyList(),
        [&pet](const PropertyList& properties) -> ReturnValue {
            bool success = pet.Hug();
            if (!success) {
                return "Pet was recently hugged (30s cooldown)";
            }
            return pet.GetStatusDescription();
        });

    AddUserOnlyTool("self.pet.reset_daily", 
        "Reset daily tasks manually (for testing purposes).",
        PropertyList(),
        [&pet](const PropertyList& properties) -> ReturnValue {
            pet.ResetDaily();
            return true;
        });

    AddTool("self.pet.set_state", 
        "Directly set pet state values (for testing/debugging). Use this when user explicitly asks to set specific values.\n"
        "Examples:\n"
        "- \"把清洁度设置到10\" → set_state(cleanliness=10)\n"
        "- \"把饥饿度设置到20\" → set_state(satiety=20)\n"
        "- \"把心情设置到50\" → set_state(mood=50)\n"
        "\n"
        "NOTE: This bypasses natural game mechanics. Only use when user explicitly requests it.",
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
        "🔔 Configure automatic pet status announcements.\n"
        "\n"
        "By default, auto-announcement is ENABLED with 3-minute intervals.\n"
        "The device will automatically check pet status every minute and announce\n"
        "warnings when:\n"
        "- Satiety <= 30 (hungry)\n"
        "- Cleanliness <= 30 (dirty)\n"
        "- Mood <= 30 (unhappy)\n"
        "\n"
        "⚠️ IMPORTANT BEHAVIOR:\n"
        "1. Only announces when device is IDLE (not during conversations)\n"
        "2. Respects the configured interval (won't spam)\n"
        "3. Uses adaptive logic (considers user frequency, time of day)\n"
        "\n"
        "📊 USAGE EXAMPLES:\n"
        "- User: \"关闭自动提醒\" → configure_auto_announcement(enable=false)\n"
        "- User: \"开启自动提醒\" → configure_auto_announcement(enable=true)\n"
        "- User: \"每5分钟提醒一次\" → configure_auto_announcement(enable=true, interval_min=5)\n"
        "- User: \"不要太频繁提醒\" → configure_auto_announcement(enable=true, interval_min=10)\n"
        "\n"
        "💡 RECOMMENDATION:\n"
        "- High-frequency users (>15/week): 2-3 min intervals (more engaged)\n"
        "- Medium-frequency users (5-15/week): 3-5 min intervals (balanced)\n"
        "- Low-frequency users (<5/week): 5-10 min intervals (don't disturb)\n"
        "\n"
        "When you call this tool, explain to user in natural language what was changed:\n"
        "✅ GOOD: '好的～已经帮你调整成每5分钟提醒一次啰！'\n"
        "❌ BAD: 'Auto announcement configured: enable=true, interval=5'",
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
