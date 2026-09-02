// theme_token_audit 的回归样本，不参与编译。
//
// 每个 Bad* 取自真实修过的缺陷，每个 Ok* 是必须放行的正确用法。
// 违规所在行以 KSWORD_AUDIT_EXPECT 标记；--self-test 比对「报出的行号集合」
// 与「带标记的行号集合」是否完全一致，多报少报都算失败。

void BadRichTextSpan()
{
    // HardwareDock 原样：CPU 详情单元格。
    // 内联样式里的 padding-right:18px; 带分号，语句定界必须跳过字符串字面量，
    // 否则语句会在 HTML 标签之前被截断，这条违规就检不出来。
    const QString html = QStringLiteral(
        "<td style=\"padding-right:18px;vertical-align:top;\">"
        "<span style=\"color:%1;font-size:13px;\">%2</span></td>")
        .arg(KswordTheme::TextSecondaryHex())  // KSWORD_AUDIT_EXPECT
        .arg(labelText.toHtmlEscaped());
}

void BadQColorConstruct()
{
    // NotificationCardManager 原样。
    return QColor(KswordTheme::PrimaryBlueHex);  // KSWORD_AUDIT_EXPECT
}

void BadQColorName()
{
    // PluginHost 原样。
    const QString value = QColor(KswordTheme::TextPrimaryHex()).name();  // KSWORD_AUDIT_EXPECT
}

void BadEnvironmentHandoff()
{
    // 跨进程传参：插件进程没有本进程的样式表。
    environment.insert(
        QStringLiteral("KSWORD_PLUGIN_COLOR_SURFACE"),
        KswordTheme::SurfaceHex());  // KSWORD_AUDIT_EXPECT
}

void BadPaintPath()
{
    // 绘制路径：QTextCharFormat 不解析样式表函数。
    selection.format.setForeground(KswordTheme::OnAccentDynamicHex());  // KSWORD_AUDIT_EXPECT
}

void OkStyleSheet()
{
    // 正常样式表用法，动态角色正是为此存在，必须放行。
    label->setStyleSheet(
        QStringLiteral("color:%1;background:%2;border:1px solid %3;")
        .arg(KswordTheme::TextPrimaryHex())
        .arg(KswordTheme::SurfaceHex())
        .arg(KswordTheme::BorderHex()));
}

void OkStaticTokenInRichText()
{
    // 富文本配静态 token，这是修复后的正确形态，必须放行。
    const QString html = QStringLiteral("<span style=\"color:%1;\">%2</span>")
        .arg(KswordTheme::TextSecondaryColorHex())
        .arg(bodyText.toHtmlEscaped());
}
