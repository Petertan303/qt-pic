# qt-pic

## 介绍

用远程绘图 api 进行画图的小软件。初衷是熟悉熟悉qt和cpp。

需要在软件目录下新建 ApiData.json 文件，内容应当是url、header和body。例如，本地的stable diffusion api，ApiData.json 应该这么写：

```json
{
	"url": "https://localhost:7860/api/v3/txt2img",
	"header": {
		"api": "1",
		"api_key": "xxxxxx"
	},
	"body": {
		"key": "",
		"prompt": "ultra realistic close up portrait ((beautiful pale cyberpunk female with heavy black eyeliner))",
		"negative_prompt": null,
		"width": "512",
		"height": "512",
		"samples": "1",
		"num_inference_steps": "20",
		"safety_checker": "no",
		"enhance_prompt": "yes",
		"seed": null,
		"guidance_scale": 7.5,
		"multi_lingual": "no",
		"panorama": "no",
		"self_attention": "no",
		"upscale": "no",
		"embeddings_model": null,
		"webhook": null,
		"track_id": null
	}
}
```

之后会添加不同api适配，目前只有一套。（而且这套api并不规范）

## 功能

- 图片预览
- 图片和参数保存
- “上一次输入的prompt”将被自动记录，并在下一次启动时填写

~~大概不是功能:~~

- 网络错误时会弹出提示框
- clear按钮可以快速清屏右边的日志
- alt + a 是绘制的快捷键
- ctrl + s 是保存tag的快捷键（如果不手动保存的话，画完一张图后也会自动保存）

![UI示例](img/QQ20241110-134204.png)

## 需要添加的新功能

- 多套api适配
- “响应式布局”
- 配置页面，快捷修改ApiData.json的内容
- 星标 prompt，快速填充
- 对比 prompt 功能：对比两个 prompt 的 tag 差异
- 效仿其他文生图模型GUI，增加历史所有记录查看功能

### 是否要将图片嵌入窗口？

目前的是打开一个独立的小窗口。用 QWidget 和 QPainter 或许能实现在主窗口内嵌式显示图片。但是这样的话布局就得花心思。而且图片尺寸不固定，需要做成能兼容各种尺寸的窗口。可能很难。