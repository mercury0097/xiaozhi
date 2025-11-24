# BMAD Core 使用指南

## 📖 什么是 BMAD Core？

**BMAD™ Core** = Business Model-Agile Development Core

这是一个集成在 Cursor IDE 中的 **AI 驱动的软件开发框架**，提供多个专业 AI Agent 协作开发。

---

## 🎯 适用场景

### ✅ 适合的项目类型
- 💻 Web 应用开发
- 📱 移动应用开发
- 🌐 前后端分离项目
- 🏢 企业级软件系统
- 📊 数据驱动型应用

### ⚠️ 不太适合的项目类型
- 🔧 嵌入式固件开发（如本项目 ESP32）
- 🎮 游戏引擎开发
- 🤖 底层驱动开发
- 📡 实时操作系统

---

## 🚀 快速开始

### 方法 1：通过命令面板

1. **打开命令面板**
   - macOS: `Cmd + Shift + P`
   - Windows/Linux: `Ctrl + Shift + P`

2. **输入 BMAD 相关命令**
   ```
   BMAD: Start Workflow
   ```

3. **选择工作流**
   - Brownfield（现有项目）
   - Greenfield（新项目）

---

### 方法 2：通过 AI 聊天（推荐）

在 Cursor 的 AI 聊天框中，使用 `@` 来调用 Agent：

```
@bmad-orchestrator 帮我分析这个项目
```

---

## 👥 可用的 AI Agents

### 1. 项目管理类

#### @pm (Project Manager - 项目经理)
**职责**：项目规划、进度管理、资源协调

**使用示例**：
```
@pm 帮我规划下一个迭代的任务
@pm 生成项目状态报告
@pm 评估项目风险
```

---

#### @po (Product Owner - 产品负责人)
**职责**：需求管理、产品规划、优先级排序

**使用示例**：
```
@po 帮我整理用户需求
@po 创建产品 Backlog
@po 优先级排序这些功能
```

---

#### @sm (Scrum Master)
**职责**：敏捷流程管理、团队协作

**使用示例**：
```
@sm 组织 Sprint 规划会议
@sm 帮我进行每日站会
@sm 回顾这个 Sprint
```

---

### 2. 技术设计类

#### @architect (架构师)
**职责**：系统架构设计、技术选型

**使用示例**：
```
@architect 帮我设计系统架构
@architect 评审这个技术方案
@architect 推荐数据库选型
```

---

#### @ux-expert (用户体验专家)
**职责**：UI/UX 设计、用户体验优化

**使用示例**：
```
@ux-expert 帮我设计用户界面
@ux-expert 优化用户流程
@ux-expert 评审这个交互设计
```

---

### 3. 开发执行类

#### @dev (开发者)
**职责**：代码实现、技术开发

**使用示例**：
```
@dev 帮我实现这个功能
@dev 重构这段代码
@dev 修复这个 Bug
```

---

#### @analyst (分析师)
**职责**：需求分析、数据分析

**使用示例**：
```
@analyst 分析这些用户需求
@analyst 帮我做竞品分析
@analyst 生成数据报告
```

---

### 4. 质量保证类

#### @qa (Quality Assurance - 质量保证)
**职责**：测试、质量检查、缺陷管理

**使用示例**：
```
@qa 帮我设计测试用例
@qa 检查代码质量
@qa 执行回归测试
```

---

### 5. 编排协调类

#### @bmad-orchestrator (编排器)
**职责**：协调多个 Agent、工作流管理

**使用示例**：
```
@bmad-orchestrator 启动项目分析工作流
@bmad-orchestrator 创建一个完整的开发计划
@bmad-orchestrator 协调团队完成这个功能
```

---

## 📋 工作流 (Workflows)

### Brownfield 工作流（现有项目）

#### 1. brownfield-fullstack.yaml
**用途**：现有全栈项目开发

**启动方式**：
```
@bmad-orchestrator 启动 brownfield fullstack 工作流
```

**流程**：
1. 分析现有代码库
2. 识别改进点
3. 创建开发任务
4. 执行开发
5. QA 检查

---

#### 2. brownfield-service.yaml
**用途**：现有后端服务开发

**适用于**：
- API 优化
- 数据库重构
- 性能提升

---

#### 3. brownfield-ui.yaml
**用途**：现有前端界面开发

**适用于**：
- UI 改版
- 交互优化
- 组件重构

---

### Greenfield 工作流（新项目）

#### 1. greenfield-fullstack.yaml
**用途**：从零开始创建全栈项目

**启动方式**：
```
@bmad-orchestrator 启动 greenfield fullstack 工作流
```

**流程**：
1. 需求分析
2. 架构设计
3. 技术选型
4. PRD 编写
5. 开发实施

---

#### 2. greenfield-service.yaml
**用途**：创建新的后端服务

---

#### 3. greenfield-ui.yaml
**用途**：创建新的前端应用

---

## 🛠️ 实用命令示例

### 项目文档生成

```
@pm 生成项目 README
@architect 生成架构文档
@qa 生成测试报告
```

---

### 需求管理

```
@po 创建用户故事
@analyst 分析用户痛点
@pm 评估开发工作量
```

---

### 代码开发

```
@dev 实现登录功能
@architect 设计数据库 Schema
@qa 编写单元测试
```

---

### 质量保证

```
@qa 执行质量检查
@architect 进行架构评审
@sm 组织代码审查
```

---

## 📚 可用模板

BMAD Core 提供了多种模板，位于 `.bmad-core/templates/`：

| 模板文件 | 用途 | 使用场景 |
|---------|------|---------|
| `prd-tmpl.yaml` | 产品需求文档 | 新功能规划 |
| `story-tmpl.yaml` | 用户故事 | 敏捷开发 |
| `architecture-tmpl.yaml` | 架构文档 | 系统设计 |
| `qa-gate-tmpl.yaml` | 质量检查清单 | 发布前检查 |
| `project-brief-tmpl.yaml` | 项目简介 | 项目启动 |
| `front-end-spec-tmpl.yaml` | 前端规范 | UI 开发 |

**使用示例**：
```
@po 使用 story-tmpl 创建一个用户故事
@architect 使用 architecture-tmpl 设计架构
```

---

## 🎯 针对本项目（xiaozhi-esp32）的使用建议

### ✅ 适合用 BMAD 做的事

#### 1. 项目文档生成
```
@pm 帮我生成 xiaozhi-esp32 项目的 README 文档
```

**输出**：
- 项目简介
- 功能列表
- 安装步骤
- 使用说明

---

#### 2. 架构分析
```
@architect 分析 xiaozhi-esp32 的系统架构，并提供优化建议
```

**输出**：
- 系统架构图
- 模块划分
- 依赖关系
- 优化建议

---

#### 3. 需求整理
```
@po 帮我整理 xiaozhi-esp32 的功能需求和待开发功能
```

**输出**：
- 功能清单
- 优先级排序
- 开发计划

---

#### 4. 质量检查
```
@qa 帮我检查 xiaozhi-esp32 的代码质量和潜在问题
```

**输出**：
- 代码质量报告
- 潜在 Bug
- 改进建议

---

### ❌ 不适合用 BMAD 做的事

#### 1. 嵌入式代码开发
- BMAD 主要面向 Web/App 开发
- 对 ESP-IDF 框架不够了解
- 不熟悉硬件相关代码

**建议**：继续使用 Claude 直接对话

---

#### 2. 实时调试
- BMAD 工作流较慢
- 不适合快速迭代

**建议**：使用 ESP-IDF 的调试工具

---

#### 3. 硬件相关问题
- BMAD 不了解 ESP32 硬件特性
- 不熟悉音频处理、MQTT 等

**建议**：查阅 ESP-IDF 官方文档或询问 Claude

---

## 📖 学习资源

### 内置文档

查看 `.bmad-core/` 目录下的文档：

```bash
# 用户指南
cat .bmad-core/user-guide.md

# 现有项目开发指南
cat .bmad-core/working-in-the-brownfield.md

# 增强 IDE 开发工作流
cat .bmad-core/enhanced-ide-development-workflow.md
```

---

### 在线帮助

在 Cursor 聊天框中：

```
@bmad-orchestrator 介绍一下你自己
@bmad-orchestrator 如何使用 BMAD Core？
@bmad-orchestrator 列出所有可用的命令
```

---

## 🎬 完整示例：使用 BMAD 生成项目文档

### Step 1：启动编排器
```
@bmad-orchestrator 帮我生成 xiaozhi-esp32 项目的完整文档
```

### Step 2：等待分析
BMAD 会：
1. 分析代码结构
2. 识别主要功能
3. 生成文档大纲

### Step 3：调用 PM 生成
```
@pm 基于分析结果，生成详细的项目文档
```

### Step 4：调用 Architect 补充
```
@architect 添加架构设计文档
```

### Step 5：调用 QA 补充
```
@qa 添加测试文档和质量报告
```

---

## ⚠️ 注意事项

### 1. BMAD 的局限性
- ✅ 擅长：Web/App 项目、文档生成、流程管理
- ❌ 不擅长：嵌入式开发、硬件相关、底层优化

### 2. 响应速度
- BMAD 工作流可能较慢（需要多个 Agent 协作）
- 简单问题建议直接问 Claude

### 3. 学习成本
- BMAD 有自己的术语和流程
- 建议先从简单命令开始

---

## 🚀 推荐使用方式

### 对于 xiaozhi-esp32 项目：

#### 使用 BMAD：
- ✅ 生成项目文档
- ✅ 整理需求清单
- ✅ 规划开发计划

#### 使用 Claude 直接对话：
- ✅ 代码实现
- ✅ Bug 修复
- ✅ 硬件调试
- ✅ 性能优化

---

## 📞 获取帮助

### 在 Cursor 中：
```
@bmad-orchestrator help
```

### 在本文档中：
- 查看具体 Agent 的使用示例
- 查看工作流说明
- 查看模板列表

---

## 🎯 总结

| 方面 | BMAD Core | Claude 直接对话 |
|------|-----------|----------------|
| **适用项目** | Web/App | 所有项目（包括嵌入式） |
| **文档生成** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **嵌入式开发** | ⭐⭐ | ⭐⭐⭐⭐⭐ |
| **响应速度** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **学习成本** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **灵活性** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |

**建议**：
- 对于**文档**、**规划**类任务 → 使用 BMAD
- 对于**代码**、**调试**类任务 → 使用 Claude 直接对话

---

**现在你可以尝试**：

```
@bmad-orchestrator 帮我分析 xiaozhi-esp32 项目
```

或

```
@pm 生成项目 README
```

**祝使用愉快！** 🎉

