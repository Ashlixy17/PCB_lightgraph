# 软件版分区调节：变更与验证记录

日期：2026-10-09。工作目录：`D:\Code project\PCB_lightgraph-main`。

## 同步与交付

- 先获取 `upstream/main` 与 `origin/main`，本地 `main` 快进至官方 v1.5.1：`d1cac6f9e6f00e307db2fdebd73e7655414e1fe4`。同步时两个远端与本地提交一致，已跟踪文件无修改。
- 功能开发使用本地 `codex/region-adjustment` 分支，源码交付目标为 `origin/main`；运行包保留在本地构建目录。
- 原有未跟踪文档 `docs/html-version-plan.md` 保留。
- 启动欢迎/免责声明弹窗已移除。
- 可运行文件：[PCB_lightgraph.exe](../build-region-adjustment/portable/PCB_lightgraph.exe)。移动时复制整个 `portable` 文件夹，Qt 插件和 DLL 需要一起保留。
- EXE 的 SHA-256：`70EBA81E5171923504FDFC314F95CF6C57E01A175D32C8469C535EC12F2B8C06`。

## 使用方法

1. 加载图片，点击“区域选择/编辑”标题展开编辑器。
2. 在工具下拉框选择矩形、套索、画笔或魔棒，在主预览中划取；“新增区域”会避开已有区域。选中区域后可以切换“增加选区”或“减少选区”，右键临时反转增减操作。
3. “选择”工具可在五个预览中点选区域。滚轮缩放、中键平移保留；选择工具也支持左键拖动平移。
4. 在“区域参数”中调整当前区域。滑块刻度沿用全局设置，小圆点表示对应的全局值。全局变化时局部按 `clamp(全局值 + 偏移)` 跟随；达到上下限不会丢失偏移，主动调整局部滑块才改变偏移。
5. 支持重命名、删除确认、复制局部调整、重置局部调整，以及最多 50 步撤销/重做。一次划取或一次连续拖动记为一步。快捷键：`Ctrl+Z`、`Ctrl+Y` / `Ctrl+Shift+Z`；`Esc` 取消当前选区任务。
6. 收起“区域选择/编辑”后恢复原来的左键布灯操作。

局部参数共 9 项：金属判定、丝印、透光、敷铜阈值；裸露基材灰度上下限及颜色相似度；边缘上下限。裸露基材、边缘处理等开关及处理方式仍由全局控制，相关局部滑块随原来的全局模式显示。不新增独立灰度模式。

魔棒以点击处的固定颜色为种子，采用四邻域连通；勾选“全图同色”时扫描全图。RGB 颜色距离容差默认为 15，范围 0～100。计算、区间扫描与归属合并采用约 8 ms 时间片；切换工具、选区或参数、退出编辑、窗口失焦和 `Esc` 均可取消，旧结果不能写入。

## 渲染与工程

- 区域使用原图像素的行区间保存，每个像素最多属于一个区域。归属缓存只保留原图与一个预览尺寸。
- 基础分层按像素归属取参数，零偏移时沿用基线处理。边缘按相同阈值分组，使用完整图像上下文计算，再限制写入所属区域，避免在选区边界产生假轮廓。
- 高光和正在划取的轮廓只绘制到预览。导出读取当前渲染代次的完整分辨率生产层。原有导出中的六个 EDA 定位像素保留。
- `.pcblg` 根版本为 2，区域子数据版本为 1。兼容无区域的旧软件工程；区域保存恢复仅保证本软件版内使用。
- 导入先解包、解析、选取声明的源图、解码并校验，全部成功后才应用。拒绝非法偏移、重复 ID、越界、重叠、尺寸不符及缺失源图；失败保留当前工程及临时源图。
- 含区域工程达到原软件的 1600 万像素限制时拒绝自动缩放。保存使用当前实际处理图的 PNG，防止原始大图与区域坐标尺寸不符。
- 新图片清空区域与历史；画图重载同尺寸图片保留区域及偏移、清空历史；尺寸改变时清空区域并提示。

核心文件：`regionmodel.*`（模型、区间、历史、序列化），`regionselection.*`（分批选区），`regionslider.*`（基准点），`mainwindowregions.cpp`（界面与交互），`imageprocessor.*` 和 `mainwindow.cpp`（分层、边缘、工程接入）。

本机 Qt 6.11.1 下，内嵌 FluentUI3Style 源码缺少外观声明的 include；`fluentstylecompat.cpp` 由工程补齐此声明，未修改第三方源码。欢迎弹窗的移除与此编译依赖分别处理。

## 实际验证

环境：Windows 11，Qt 6.11.1，MinGW GCC 13.1.0，64 位 Release。Qt 5 构建分支保留，未在本机验证 Qt 5 运行。

| 验证 | 结果与证据 |
| --- | --- |
| 产品构建 | `build-region-adjustment/build.log`，qmake / mingw32-make 成功 |
| 运行包 | windeployqt 打包成功；移除 PATH 中的 Qt / MinGW 后启动保持运行，加载模块均来自 `portable`；记录见 `build-region-adjustment/startup-results.txt` |
| 原有渐进渲染测试 | 12 passed、0 failed，131 ms；包含初始化和清理。见 `build-region-progressive-tests/progressive-results.txt` |
| 新增区域验证 | 23 passed、0 failed，13,516 ms；包含初始化和清理。见 `build-region-tests/regions-results.txt` |
| 独立像素参考 | 从固定 v1.5.1 提交提取原始 ImageProcessor，覆盖全部 7 种阻焊色、3 种表面工艺及两种裸露基材模式，逐像素比较 |
| 选区与历史 | 区间并集/差集的独立像素参考、区域互斥、魔棒固定种子参考、偏移钳位恢复、撤销/重做上限及拖动一步 |
| 渲染边界 | 9 项参数分别与完整图像上下文参考比较；零偏移一致、区域外生产像素不变、边缘无选区假轮廓、高光不进入层与实际 PNG 导出 |
| 窗口交互 | Qt 真窗口中的鼠标按下/移动/释放、右键增减反转、五预览点选、缩放/平移、退出编辑后布灯、魔棒取消和空选区；使用 QtTest 自动输入事件 |
| 元数据操作 | 实际对话框验证重命名、复制、重置、删除取消/确认及撤销 |
| 工程 | 实际 ZIP 工程往返、损坏数据、指定源图与额外图片、源图缺失、超限拒绝且保留当前工程 |
| 画图重载 | 同尺寸保留、尺寸改变提示清空、新图片清空 |
| 界面 | 75% / 100% / 150% 字体尺寸、100 字长名称；截图检查和控制台无横向裁切断言通过 |

截图：[编辑工具](../build-region-tests/regions-ui.png)、[局部滑块与基准点](../build-region-tests/regions-local-parameters.png)、[150% 字体](../build-region-tests/regions-ui-20.png)。

接近上限的场景采用 `4000 × 3999`（15,996,000 像素）实图，三个区域有局部偏移，默认关闭边缘与灯光。最后一次运行：交互预览 49 ms、完整分辨率分层 2,377 ms、全图魔棒取消响应 93 ms。整个测试进程峰值工作集为 824.9 MiB，记录见 `build-region-tests/memory-results.txt`。这些数字是本机该场景的测量值；大量不同边缘阈值分组会增加完整上下文计算次数。

## 复现命令

在项目根目录使用 PowerShell：

```powershell
.\tests\run-regions.ps1

$env:PATH = 'D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
Push-Location .\build-region-adjustment
qmake ..\PCB_lightgraph.pro CONFIG+=release
mingw32-make -j4
Pop-Location

Push-Location .\build-region-progressive-tests
qmake ..\tests\progressive_rendering_tests.pro CONFIG+=release CONFIG+=c++17
mingw32-make -j4
.\release\progressive_rendering_tests.exe -o progressive-results.txt,txt
Pop-Location
```

`run-regions.ps1` 自动创建测试构建目录，并从固定基线提交提取参考实现。构建与运行包位于现有 Git 忽略的构建目录中，测试记录未加入暂存区。

2026-10-09 界面修正：工具按钮改为下拉选择；参考点与 Fluent 滑轨统一使用浮点中心，修复整数坐标造成的半像素偏上。重新构建、区域测试与截图检查通过。
