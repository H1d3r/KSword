#include "ThemeStatusRole.h"

#include "../theme.h"

#include <QLatin1String>
#include <QStyle>
#include <QVariant>
#include <QWidget>

namespace ks::ui
{
    namespace
    {
        // StatusRoleName 作用：把枚举映射成属性值文本。
        // 这里的拼写与 BuildStatusRoleStyleRules 的选择器共用同一组字面量，两处必须同时改。
        QLatin1String StatusRoleName(const StatusRole role)
        {
            switch (role)
            {
            case StatusRole::Idle:
                return QLatin1String("idle");
            case StatusRole::Info:
                return QLatin1String("info");
            case StatusRole::Success:
                return QLatin1String("success");
            case StatusRole::Warning:
                return QLatin1String("warning");
            case StatusRole::Error:
                return QLatin1String("error");
            case StatusRole::None:
                break;
            }
            return QLatin1String("");
        }
    }

    void ApplyStatusRole(QWidget* const widget, const StatusRole role)
    {
        if (widget == nullptr)
        {
            return;
        }
        const QString roleText(StatusRoleName(role));
        // 状态未变化时必须直接返回：采集回调会在每轮刷新里把状态标签重置成同一状态，
        // 而 unpolish/polish 要重算整条样式规则链，高频事件下足以拖慢刷新。
        if (widget->property(kStatusRoleProperty).toString() == roleText)
        {
            return;
        }
        widget->setProperty(kStatusRoleProperty, roleText);
        // 属性选择器只在 polish 阶段参与匹配；改完属性不重新 polish 就不会换色。
        QStyle* const widgetStyle = widget->style();
        if (widgetStyle != nullptr)
        {
            widgetStyle->unpolish(widget);
            widgetStyle->polish(widget);
        }
        widget->update();
    }

    QString BuildStatusRoleStyleRules()
    {
        // 选择器组与「{」写在同一字符串片段内，i18n 审计才能识别为 QSS 而非 UI 文本。
        // 空闲态直接用动态调色板角色：它本来就跟随主题，不需要每次重建时求值。
        // 其余四色是 palette 表达不了的语义色，随本样式块整体重建取当前主题取值。
        return QStringLiteral(
            "QLabel[ksword_status_role=\"idle\"]{"
            "  color:palette(placeholder-text);"
            "  font-weight:600;"
            "}"
            "QLabel[ksword_status_role=\"info\"]{"
            "  color:%1;"
            "  font-weight:600;"
            "}"
            "QLabel[ksword_status_role=\"success\"]{"
            "  color:%2;"
            "  font-weight:600;"
            "}"
            "QLabel[ksword_status_role=\"warning\"]{"
            "  color:%3;"
            "  font-weight:600;"
            "}"
            "QLabel[ksword_status_role=\"error\"]{"
            "  color:%4;"
            "  font-weight:600;"
            "}")
            .arg(KswordTheme::InfoHex())
            .arg(KswordTheme::SuccessHex())
            .arg(KswordTheme::WarningHex())
            .arg(KswordTheme::ErrorHex());
    }
}
