# 文档整合脚本
# 该脚本将合并重复或冗余的文档

# 设置错误处理
$ErrorActionPreference = "Stop"

Write-Host "=== 开始文档整合 ===" -ForegroundColor Green
Write-Host ""

# 步骤1: 创建文档备份目录
$backupDir = "e:\Software_Structure\docs\backup_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
New-Item -Path $backupDir -ItemType Directory -Force | Out-Null
Write-Host "创建文档备份目录: $backupDir" -ForegroundColor Yellow

# 步骤2: 备份要合并的文档
$docsToConsolidate = @(
    "移植指南.md",
    "综合移植指南.md",
    "新硬件移植指南.md",
    "显示驱动指南.md",
    "FM33LC0xx_矩阵显示驱动使用指南.md",
    "TM1681驱动移植指南.md"
)

foreach ($doc in $docsToConsolidate) {
    $sourcePath = "e:\Software_Structure\docs\$doc"
    $targetPath = "$backupDir\$doc"
    Copy-Item -Path $sourcePath -Destination $targetPath -Force
    Write-Host "备份文档: $doc" -ForegroundColor Cyan
}

Write-Host ""
Write-Host "已备份所有要整合的文档到: $backupDir" -ForegroundColor Green
Write-Host ""
Write-Host "=== 文档整合计划 ===" -ForegroundColor Green
Write-Host "1. 整合'移植指南.md'和'综合移植指南.md'为新的'移植指南_新.md'" -ForegroundColor Cyan
Write-Host "2. 将'新硬件移植指南.md'内容合并到'移植指南_新.md'" -ForegroundColor Cyan
Write-Host "3. 整合显示驱动相关的三个文档为新的'显示驱动指南_新.md'" -ForegroundColor Cyan
Write-Host ""
Write-Host "注意: 此脚本只进行文档备份，整合工作需要人工操作以确保内容的准确性和连贯性。" -ForegroundColor Yellow
Write-Host ""
Write-Host "=== 建议的整合后文档结构 ===" -ForegroundColor Green
Write-Host "1. 移植指南_新.md - 完整的移植流程与指南，包含新硬件移植流程" -ForegroundColor Cyan
Write-Host "2. 显示驱动指南_新.md - 完整的显示驱动设计、使用与移植指南" -ForegroundColor Cyan
Write-Host "3. 分层软件架构设计.md - 保持不变" -ForegroundColor Cyan
Write-Host "4. 接口层次化重构迁移指南.md - 保持不变" -ForegroundColor Cyan
Write-Host "5. 系统架构设计.md - 保持不变" -ForegroundColor Cyan
Write-Host "6. API使用指南.md - 保持不变" -ForegroundColor Cyan
Write-Host "7. 电源管理模块使用指南.md - 保持不变" -ForegroundColor Cyan
Write-Host ""
Write-Host "=== 操作完成 ===" -ForegroundColor Green
