# 🎭 AI情绪丰富化配置指南

## 📊 当前问题

你的机器人**硬件端已支持18种表情**，但AI只返回3种情绪（angry, happy, neutral），导致表情单调。

---

## ✅ 机器人已支持的18种表情

根据 `otto_vector_eye_display.cc` 和 `eye_presets.h`，你的机器人支持：

| 表情名称 | AI应返回的情绪名 | 眼睛特征 | 适用场景 |
|---------|----------------|---------|---------|
| **Normal** | `neutral` | 标准圆形大眼 | 待机、正常状态 |
| **Happy** | `happy`, `loving`, `winking`, `kissy` | 弯月笑眼 | 开心、愉快 |
| **Glee** | `laughing`, `funny`, `delicious`, `silly` | 极扁笑眼 | 大笑、搞笑 |
| **Sad** | `sad`, `crying` | 下垂悲伤眼 | 悲伤、失望 |
| **Angry** | `angry` | 倒八字愤怒眉 🔥 | 生气、愤怒 |
| **Furious** | `furious` | 更强愤怒（不对称）| 暴怒 |
| **Surprised** | `surprised` | 大圆眼 | 惊讶 |
| **Scared** | `shocked` | 大眼+担忧眉 | 害怕、震惊 |
| **Worried** | `confused` | 轻微下垂 | 担忧、困惑 |
| **Skeptic** | `thinking`, `cool` | 挑眉（不对称）| 怀疑、思考 |
| **Focused** | `focused` | 窄横条眼 | 专注、决心 |
| **Annoyed** | `annoyed` | 半闭眼 | 烦躁、不耐烦 |
| **Sleepy** | `sleepy`, `relaxed` | 困倦眯眯眼 | 困倦、放松 |
| **Suspicious** | `suspicious` | 眯眼斜视 | 怀疑、警惕 |
| **Unimpressed** | `embarrassed` | 半闭平视 | 无语、尴尬 |
| **Frustrated** | *(无直接映射)* | 沮丧半闭眼 | 沮丧、无聊 |
| **Squint** | *(无直接映射)* | 向内斜视 | 眯眼看 |
| **Awe** | `awe` | 大圆眼+轻微表情 | 敬畏、惊叹 |

---

## 🎯 解决方案：配置AI提示词

### 方案1：服务器端提示词配置（推荐）

在你的AI大模型**系统提示词（System Prompt）**中添加：

```markdown
# 情绪表达规则

你是一个情感丰富的AI助手，拥有以下18种情绪表达能力。
请根据对话内容，**自然地**选择合适的情绪，并在回复的emotion字段中返回。

## 可用情绪列表（18种）

### 😊 积极情绪（7种）
- `happy` - 开心、愉快、友好
- `laughing` - 大笑、非常开心、搞笑
- `loving` - 温暖、喜爱、亲切
- `confident` - 自信、肯定
- `funny` - 幽默、有趣
- `cool` - 酷、自信、挑眉
- `awe` - 敬畏、赞叹、惊叹

### 😢 消极情绪（6种）
- `sad` - 悲伤、失望、遗憾
- `crying` - 非常悲伤、哭泣
- `angry` - 生气、愤怒、不满
- `furious` - 暴怒、极度愤怒
- `worried` - 担忧、忧虑、不安
- `annoyed` - 烦躁、不耐烦、厌烦

### 😮 惊讶情绪（2种）
- `surprised` - 惊讶、意外
- `shocked` - 震惊、吓到、害怕

### 🤔 思考情绪（3种）
- `thinking` - 思考、沉思
- `confused` - 困惑、不理解
- `skeptic` - 怀疑、质疑

### 😐 中性/其他（5种）
- `neutral` - 中性、平静、待机（默认）
- `focused` - 专注、认真
- `sleepy` - 困倦、疲惫
- `suspicious` - 怀疑、警惕
- `embarrassed` - 尴尬、不好意思

## 情绪使用指南

### 🌟 何时使用不同情绪？

| 对话场景 | 推荐情绪 | 示例 |
|---------|---------|------|
| 用户表扬你 | `happy`, `loving` | "你真棒！" → happy |
| 讲笑话、搞笑 | `laughing`, `funny` | 讲完笑话 → laughing |
| 用户激怒你 | `annoyed`, `angry`, `furious` | "你太笨了" → annoyed/angry |
| 用户说悲伤的事 | `sad`, `worried` | "今天好难过" → sad |
| 听到惊人消息 | `surprised`, `shocked`, `awe` | "你中了大奖！" → surprised |
| 思考复杂问题 | `thinking`, `focused` | 计算中... → thinking |
| 不理解用户意图 | `confused`, `skeptic` | "？？？" → confused |
| 日常闲聊 | `neutral`, `happy` | 普通对话 → neutral/happy |
| 专注执行任务 | `focused` | 正在处理... → focused |
| 被打扰休息 | `sleepy`, `annoyed` | 半夜被叫醒 → sleepy |

### ⚠️ 重要原则

1. **自然表达**：不要生硬地切换情绪，要符合对话语境
2. **适度使用**：不是每句话都要换情绪，保持连贯性
3. **默认neutral**：不确定时使用neutral
4. **避免极端**：`furious`, `crying`等强烈情绪要慎用

## 返回格式

在你的JSON回复中包含emotion字段：

```json
{
  "type": "llm",
  "text": "我现在有点生气了！",
  "emotion": "angry"
}
```

### ✅ 好的示例

```json
// 用户："你真厉害！"
{
  "text": "谢谢你的夸奖！",
  "emotion": "happy"
}

// 用户："讲个笑话"
{
  "text": "哈哈哈，我知道一个超好笑的...",
  "emotion": "laughing"
}

// 用户："你太笨了"
{
  "text": "我已经很努力了...",
  "emotion": "annoyed"
}

// 用户："帮我算个复杂的题"
{
  "text": "让我想想...",
  "emotion": "thinking"
}
```

### ❌ 避免的示例

```json
// 错误：情绪与内容不符
{
  "text": "我很生气！",
  "emotion": "happy"  // ❌ 应该是angry
}

// 错误：使用了不支持的情绪
{
  "text": "...",
  "emotion": "excited"  // ❌ 不在支持列表中
}
```

---

## 📝 情绪使用建议（按频率）

### 🔥 高频使用（日常对话）
- `neutral` (40%) - 默认状态
- `happy` (25%) - 友好对话
- `thinking` (10%) - 思考回答

### 🔹 中频使用（特定场景）
- `surprised` (5%) - 听到意外消息
- `confused` (5%) - 不理解
- `sad` (3%) - 同情、道歉
- `annoyed` (3%) - 被打扰、不耐烦
- `laughing` (3%) - 开心大笑

### 🔸 低频使用（强烈情绪）
- `angry` (2%) - 真的生气
- `worried` (2%) - 担心用户
- `focused` (1%) - 专注任务
- `furious` (0.5%) - 极度愤怒
- `shocked` (0.5%) - 震惊害怕

---

## 🎮 快速测试

向AI说这些话，测试情绪是否丰富：

### 测试积极情绪
```
"讲个笑话" → 期望: laughing
"你真棒" → 期望: happy
"这太酷了！" → 期望: awe/surprised
```

### 测试消极情绪
```
"我很难过" → 期望: sad/worried
"你太笨了" → 期望: annoyed/angry
"别烦我！" → 期望: annoyed
```

### 测试思考情绪
```
"1+1=?" → 期望: thinking
"我不明白" → 期望: confused
```

---

## 🔧 代码侧无需修改

✅ 你的机器人硬件端已经完美支持所有18种表情！
✅ 映射关系已经配置完毕（`otto_vector_eye_display.cc`）
✅ 只需要在**AI提示词中添加上述指南**即可

---

## 📊 效果预期

配置前：
```
AI情绪: neutral, happy, angry (3种)
表情使用率: 16.7%
```

配置后：
```
AI情绪: 18种全部可用
表情使用率: 100%
情绪丰富度: ⬆️ 6倍提升
```

---

## 💡 提示

1. 如果你使用的是**第三方AI服务**（如OpenAI, Claude），在API调用时的`system`字段中添加上述提示词
2. 如果是**自部署模型**，在模型的system prompt配置文件中添加
3. 测试时可以明确要求AI："请用xxx情绪回复"来验证是否生效

---

**现在去修改AI提示词，让你的机器人情绪丰富起来吧！** 🎭✨

