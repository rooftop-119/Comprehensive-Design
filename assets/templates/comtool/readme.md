#### 原模板地址

[基于qmake构建的Qt项目：串口助手comtool](https://github.com/feiyangqingyun/QWidgetDemo/tree/7d4d159c7e210b02fd8c9f050bf03fba522b1162/tool/comtool)

#### 修改说明

| 版本   | 修改说明                                                     |
| ------ | ------------------------------------------------------------ |
| `V1.0` | 第三方`QExtSerialPort`组件改为官方支持的`QSerialPort`组件。  |
| `V1.1` | 将`qmake`改为官方更推荐的`cmake`。<br />为简化项目结构`api` 和`form`不再独立成库。<br />添加了第三方组件[Qcustomplot](./lib/qcustomplot)，预备作为绘图工具，但暂未使用。 |

#### 版本更新说明

##### `V1.0`→`V1.1`

* 删除老版本中所有的`.pro`和`.pri`文件
* 在项目根目录下添加[CMakeLists.txt](./CMakeLists.txt)文件

> 如在`V1.0`版本基础上已作开发，则版本迁移时，除了上述步骤，可能还需要在`CMakeLists.txt`中对相应部分作出修改。

#### 环境说明

请在 `MaintenanceTool.exe`中自主添加组件：`Qt 5 Compatibility module`,`Qt Serial Bus`,`Qt Serial Port`

#### 其他说明

第三方组件`Qcustomplot`的加入会拖慢编译速度，可暂时注释或删除掉`CMakeLists.txt`中有关`PrintSupport`和`qcustomplot`的部分。

（暂不需要）编译后请将源码下的file目录中的所有文件复制到可执行文件同一目录。