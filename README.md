# Comprehensive-Design
63组综设代码管理
大家更新代码前记得先pull再push，不然会冲突

| 位置                                         | 建议内容   | 说明                          |
| :------------------------------------------- | :--------- | :---------------------------- |
| [`./assets/images/`](./assets/images/)       | 图片资源   | 界面截图、图标、设计素材等    |
| [`./assets/videos/`](./assets/videos/)       | 视频资源   | 演示视频、教程录像等          |
| [`./assets/templates/`](./assets/templates/) | 模板文件   | 项目模板、配置模板等          |
| [`./docs/`](./docs/)                         | 项目文档   | 设计文档、用户手册、API文档等 |
| [`./releases/`](./releases/)                 | 发布版本   | 各版本的可执行程序打包文件    |
| [`./src/firmware/`](./src/firmware/)         | 下位机源码 | 嵌入式固件、硬件相关代码      |
| [`./src/software/`](./src/software/)         | 上位机源码 | 桌面应用程序、软件代码        |

### 🧩1️⃣本地初始化 Git 仓库(首次使用) 

```bash
#克隆团队仓库
git clone https://github.com/san10086/Comprehensive-Design.git

#同步远程仓库最新内容
git pull origin main

#创建自己的分支开发
git checkout -b dev-xiaoming
```

后续在自己的分支上开发。

### 🧩2️⃣提交更改到远程仓库

拉取团队最新代码（如果中间远程仓库被别人更新了，请先跳转到🧩4️⃣，然后再回到此处）。

```bash
git checkout dev-xiaoming
git pull origin dev-xiaoming
```

如果出现合并冲突，请手动解决后再执行：

```bash
git add .
git commit -m "更新说明"

#推送到远程仓库
git push origin dev-xiaoming
```

### 🧩3️⃣申请同步到主分支

1. 打开团队 GitHub 仓库；
2. 点击 **“Compare & pull request”**；
3. 填写说明
4. 提交审核；
5. 审核通过后合并到主分支。

### 🧩4️⃣开发期间保持同步

开发期间，远程仓库可能被别人更新了。这时可以执行：

```bash
git checkout main
git pull origin main
git checkout dev-xiaoming
git merge main
```

这可以让你的分支保持最新。
