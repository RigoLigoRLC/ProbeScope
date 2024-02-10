<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>SymbolBackend</name>
    <message>
        <location filename="../src/symbolbackend.cpp" line="33"/>
        <source>No error</source>
        <translation>没有错误</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="35"/>
        <location filename="../src/symbolbackend.cpp" line="154"/>
        <source>GDB not started</source>
        <translation>GDB 未启动</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="37"/>
        <source>GDB response timeout</source>
        <translation>GDB 响应超时</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="39"/>
        <location filename="../src/symbolbackend.cpp" line="77"/>
        <source>GDB startup failed</source>
        <translation>GDB 启动失败</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="41"/>
        <source>Unknown error</source>
        <translation>未知错误</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="51"/>
        <source>GDB executable path not set</source>
        <translation>GDB 可执行文件路径未设置</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="52"/>
        <source>GDB executable path is not set. Please set it in settings, and manually start GDB from menu after this.</source>
        <translation>GDB 可执行文件路径未设置。请在设置窗口中设定它，然后从菜单中手动启动 GDB。</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="78"/>
        <source>GDB startup failed. Please set proper GDB executable path and manually start GDB from menu.

Failed GDB executable path: %1
</source>
        <translation>GDB 启动失败。请设置正确的 GDB 可执行文件路径，并从菜单中手动启动 GDB。

失败的 GDB 可执行文件路径：%1
</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="93"/>
        <source>Loading symbol file...</source>
        <translation>加载符号文件…</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="103"/>
        <source>Refreshing symbols...</source>
        <translation>正在刷新符号…</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="154"/>
        <source>GDB is not started. Please start GDB from menu.</source>
        <translation>GDB 未启动。请从菜单中启动 GDB。</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="172"/>
        <source>GDB command unsuccessful</source>
        <translation>GDB 命令未成功</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="173"/>
        <source>The underlying GDB command was unsuccessful, status: %1.
Do you want to retry?

Command: %2</source>
        <translation>内部 GDB 命令未成功执行。状态：%1。
你想要重试吗？

命令：%2</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="190"/>
        <source>GDB Command Error</source>
        <translation>GDB 命令错误</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="191"/>
        <source>GDB command error: %1

Command: %2</source>
        <translation>GDB 命令错误：%1

命令：%2</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="225"/>
        <source>GDB crashed</source>
        <translation>GDB 已崩溃</translation>
    </message>
    <message>
        <location filename="../src/symbolbackend.cpp" line="226"/>
        <source>GDB crashed (exit code %1). Do you want to restart it?</source>
        <translation>GDB 已崩溃（退出代码：%1）。你想要重新启动它吗？</translation>
    </message>
</context>
<context>
    <name>SymbolPanel</name>
    <message>
        <location filename="../ui/symbolpanel.ui" line="14"/>
        <source>Form</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../ui/symbolpanel.ui" line="48"/>
        <source>Symbol file: </source>
        <translation>符号文件： </translation>
    </message>
    <message>
        <location filename="../ui/symbolpanel.ui" line="55"/>
        <source>(Not selected)</source>
        <translation>（未选择）</translation>
    </message>
    <message>
        <location filename="../ui/symbolpanel.ui" line="94"/>
        <source>Open...</source>
        <translation>打开…</translation>
    </message>
    <message>
        <location filename="../ui/symbolpanel.ui" line="107"/>
        <source>Reload</source>
        <translation>重新加载</translation>
    </message>
    <message>
        <location filename="../ui/symbolpanel.ui" line="129"/>
        <source>Symbol tree</source>
        <translation>符号树</translation>
    </message>
    <message>
        <location filename="../ui/symbolpanel.ui" line="168"/>
        <source>Add watch entry</source>
        <translation>添加监视项</translation>
    </message>
</context>
<context>
    <name>WorkspaceModel</name>
    <message>
        <location filename="../src/workspacemodel.cpp" line="32"/>
        <source>Cannot open cache file</source>
        <translation>无法打开缓冲文件</translation>
    </message>
    <message>
        <location filename="../src/workspacemodel.cpp" line="33"/>
        <source>Cache file %1 cannot be opened with read/write access.
Do you wish to choose another place for cache file?</source>
        <translation>无法以读写权限打开缓冲文件“%1”。
你想要手动为缓冲文件指定一个位置吗？</translation>
    </message>
    <message>
        <location filename="../src/workspacemodel.cpp" line="46"/>
        <source>Select where to save cache file...</source>
        <translation>选择保存缓冲文件的位置…</translation>
    </message>
    <message>
        <location filename="../src/workspacemodel.cpp" line="72"/>
        <source>Cannot map cache file</source>
        <translation>无法映射缓冲文件</translation>
    </message>
    <message>
        <location filename="../src/workspacemodel.cpp" line="73"/>
        <source>Cache file cannot be mapped to system memory. ProbeScope will now quit.</source>
        <translation>无法将缓冲文件映射至系统内存。ProbeScope 即将退出。</translation>
    </message>
    <message>
        <location filename="../src/workspacemodel.cpp" line="83"/>
        <source>Invalid GDB executable</source>
        <translation>非法的 GDB 可执行文件</translation>
    </message>
    <message>
        <location filename="../src/workspacemodel.cpp" line="84"/>
        <source>The GDB executable found in configuration is invalid.
You must specify a suitable GDB executable now.

Press Cancel to exit.</source>
        <translation>配置的 GDB 可执行文件无法使用。你必须现在指定一个适合的 GDB 可执行文件。

点击“取消”退出。</translation>
    </message>
    <message>
        <location filename="../src/workspacemodel.cpp" line="88"/>
        <source>Choose GDB suitable for embedded device</source>
        <translation>选择适用于嵌入式设备的 GDB</translation>
    </message>
</context>
</TS>
