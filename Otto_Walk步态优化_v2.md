# Otto Walk步态优化 v2 - 交替步态 ❌ 失败

## 📋 修改记录

**修改时间**: 2025-10-29  
**修改原因**: 将左右脚从"同相滑步"改为"反相交替"，实现更自然的行走  
**测试结果**: ❌ **失败 - 无法正常行走，已回退**  
**回退时间**: 2025-10-29

---

## 🔄 原始参数（备份）

```cpp
void Otto::Walk(float steps, int period, int dir, int amount) {
    int A[SERVO_COUNT] = {30, 30, 30, 30, 0, 0};
    int O[SERVO_COUNT] = {0, 0, 5, -5, HAND_HOME_POSITION - 90, HAND_HOME_POSITION};
    double phase_diff[SERVO_COUNT] = {
        0,                      // LEFT_LEG
        0,                      // RIGHT_LEG
        DEG2RAD(dir * -90),     // LEFT_FOOT: -90°
        DEG2RAD(dir * -90),     // RIGHT_FOOT: -90° (同相)
        0, 0
    };
}
```

**特点**: 
- 左右脚同相（-90°, -90°）
- 类似企鹅的滑步步态
- 两只脚同时向前、同时向后

---

## ✨ 优化后参数（v2）

```cpp
void Otto::Walk(float steps, int period, int dir, int amount) {
    int A[SERVO_COUNT] = {30, 30, 30, 30, 0, 0};
    int O[SERVO_COUNT] = {0, 0, 5, -5, HAND_HOME_POSITION - 90, HAND_HOME_POSITION};
    double phase_diff[SERVO_COUNT] = {
        0,                      // LEFT_LEG
        0,                      // RIGHT_LEG
        DEG2RAD(dir * -90),     // LEFT_FOOT: -90°
        DEG2RAD(dir * 90),      // RIGHT_FOOT: +90° (反相！)
        0, 0
    };
}
```

**改动点**: 
- `RIGHT_FOOT` 从 `-90°` 改为 `+90°`
- 左右脚反相，形成交替迈步

**预期效果**:
- 真正的交替行走
- 更自然的步态
- 可能更快、更稳定

---

## 🔧 快速回滚方法

### 方法1: 手动修改（最快）

打开 `main/boards/otto-robot/otto_movements.cc`，找到第275行：

```cpp
// 如果效果不好，改回这个：
double phase_diff[SERVO_COUNT] = {0, 0, DEG2RAD(dir * -90), DEG2RAD(dir * -90), 0, 0};

// 如果效果好，保持这个：
double phase_diff[SERVO_COUNT] = {0, 0, DEG2RAD(dir * -90), DEG2RAD(dir * 90), 0, 0};
```

### 方法2: 使用Git回滚

```bash
cd /Users/machenyang/Desktop/xiaozhi-esp32-main
git diff main/boards/otto-robot/otto_movements.cc  # 查看改动
git checkout main/boards/otto-robot/otto_movements.cc  # 恢复原始版本
```

---

## 📊 步态对比

### 原始步态（同相滑步）
```
时间:       0°      90°     180°    270°    360°
LEFT_FOOT:  后  →   中   →  前   →   中   →  后
RIGHT_FOOT: 后  →   中   →  前   →   中   →  后
            └────两脚同时动，类似企鹅────┘
```

### 优化步态（交替迈步）
```
时间:       0°      90°     180°    270°    360°
LEFT_FOOT:  后  →   中   →  前   →   中   →  后
RIGHT_FOOT: 前  →   中   →  后   →   中   →  前
            └────交替迈步，更自然────┘
```

---

## 🧪 测试建议

1. **编译烧录**: `idf.py build flash`
2. **通过MCP工具测试**:
   - `self.otto.walk_forward`: steps=3, speed=1000
   - 观察行走是否更流畅
3. **对比观察**:
   - 速度: 是否更快？
   - 稳定性: 是否更稳？
   - 自然度: 看起来是否更像人走路？

---

## 📝 评估标准

✅ **效果好的标志**:
- 行走速度加快
- 左右脚明显交替
- 重心转移更自然
- 转向更灵活

❌ **效果不好的标志**:
- 走路变得不稳
- 出现打滑
- 无法前进
- 左右摇晃过大

**如果出现❌的情况，立即回滚！**

---

## 📌 备注

- 振幅(A)和偏移(O)参数保持不变
- 只改变右脚的相位差: -90° → +90°
- 这是最小改动，风险较低
- 后续可以根据效果继续优化振幅配比

