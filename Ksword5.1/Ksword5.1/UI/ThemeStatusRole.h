#pragma once

// ============================================================
// ThemeStatusRole.h
// 作用：
// 1) 把「空闲/信息/成功/警告/错误」五种语义状态色从各页面构造期的 styleSheet 里剥离；
// 2) 控件只记录「自己处于什么状态」，颜色由全局样式块统一下发，主题切换自动跟随；
// 3) 取代 MonitorDock 家族三份重复的 monitor*ColorHex() + buildStatusStyle() 写法。
//
// 背景：语义状态色是 QPalette 角色表达不了的一类颜色 —— palette 里没有「错误红」，
// 因此不能像表面色、边框色那样直接换成 palette(...) 动态 token。但它同样不该在控件
// 构造期被 *Color().name() 烧成固定 #RRGGBB：深浅主题下 SuccessColor/ErrorColor 取值
// 不同，烧死之后状态文字会停在旧主题的红绿橙上；而控件自身 styleSheet 优先级高于
// 祖先全局 QSS，重建全局样式块也救不回来。
//
// 解法：控件侧只写状态，不写颜色。颜色集中在全局样式块的 QLabel[ksword_status_role="..."]
// 规则里，随主题刷新整块重建，控件自身不再持有 styleSheet，也就不必再被
// ThemeColorRemap 的存量样式表扫描覆盖。
// ============================================================

#include <QString>

class QWidget;

namespace ks::ui
{
    // StatusRole 作用：语义状态枚举，与全局样式块中的属性取值一一对应。
    enum class StatusRole
    {
        // None：清除状态标记，控件回落到继承色。
        None,
        // Idle：未启动、未订阅、未选择等中性态，取次级文字色。
        Idle,
        // Info：进行中、已提交、等待回执等中性提示。
        Info,
        // Success：已生效、已连接、校验通过。
        Success,
        // Warning：降级可用、部分失败、结果不完整。
        Warning,
        // Error：失败、不可用、被拒绝。
        Error
    };

    // kStatusRoleProperty 作用：承载状态的动态属性名，全局样式块按它做属性选择。
    inline constexpr const char* kStatusRoleProperty = "ksword_status_role";

    // ApplyStatusRole 作用：把语义状态写到控件上，并让样式立刻重新匹配。
    // 输入：目标控件（空指针直接返回）与语义状态。
    // 处理：属性值未变化时直接返回，避免无谓的 unpolish/polish；变化时改属性再重新
    //       polish —— Qt 的属性选择器只在 polish 阶段参与匹配，改完属性不 polish 不换色。
    // 调用方式：取代原先的 label->setStyleSheet(buildStatusStyle(monitor*ColorHex()))。
    void ApplyStatusRole(QWidget* widget, StatusRole role);

    // BuildStatusRoleStyleRules 作用：生成语义状态色的全局 QSS 规则片段。
    // 返回：可直接拼进基础控件样式块的规则文本，颜色取当前主题的语义色。
    // 调用方式：BuildGlobalBaseControlStyleBlock 内部拼接，随主题刷新整块重建。
    QString BuildStatusRoleStyleRules();
}
