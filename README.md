# qt-pic

## 介绍

用远程绘图 api 进行画图的小软件。目的是让我自己熟悉熟悉qt和cpp，换言之就是练手用的。

![UI示例](img/QQ20241110-134204.png)

需要在软件目录下新建 ApiData.json 文件，内容应当包含url、header和body。例如，本地的stable diffusion api，ApiData.json 应该这么写：

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

之后会添加不同api适配，目前只有一套。（而且这套api并不规范）。

## 功能

- 图片预览
- 图片和参数保存
- “上一次输入的prompt”将被自动记录，并在下一次启动时填写
- 收藏prompt，查看与管理收藏的prompt

大概不是功能:

- 网络错误时会弹出提示框
- clear按钮可以快速清屏右边的日志
- alt + s 是绘制的快捷键
- ctrl + s 是保存tag的快捷键（如果不手动保存的话，画完一张图后也会自动保存）
- 响应式布局

## 需要添加的新功能

- 多套api适配
- 配置页面
	- 快捷修改ApiData.json的内容（也就是除prompt之外请求的参数）
	- 考虑使用tab容器
	- 难点在于每个api都有不同的请求参数，或许无法自动生成美观的ui页面
- 历史所有记录查看
	- 考虑使用tab容器
	- 考虑使用新窗口
	- 效仿其他文生图模型GUI来做
- 星标 prompt
	- 填充的时候，当前的prompt不应消失，而是应当形成一个独立的悬浮窗口，可以选择关闭，也可以选择收藏、回归等功能
- 对比 prompt 功能
	- 考虑使用tab容器
	- 考虑使用新窗口
	- 对比两个 prompt 的 tag 差异
	- 应当如同git对比diff一样对比差异，但不应完全一样。对于prompt而言，顺序也算差异
	- 有点难办
	
2024-12-29 新增需求：

- 备份星标prompt
	- 管理备份得到的prompt
	- 增加prompt版本库：
		- 同一title不同prompt按不同时间归类排序
- 星标prompt首字母快捷搜索
	- 例如，按下a，快捷跳转到下一个标题首字母为a的prompt页面
	- 短时间内输入miku，跳转到下一个前四字母为miku的页面

2025-02-03 新增需求：

- 