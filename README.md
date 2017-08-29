AnimatePacker4
=============

The  2D frame animation editor for cocos2dx.

- Works without extra library.
- WYSIWYG(what you see is what you get) editing
- Suitable for cocos2dx 3.0+ or any version which has `AnimationCache` class.
- Quick and safe.

## Usage

1. Download AnimatePacker from [release](https://github.com/hellokenlee/AnimatePacker/releases)
1. Use [TexturePacker](https://www.codeandweb.com/texturepacker) to pack all your animation frames into one `*.plist` and `*.png`
2. Drag your plist file into AnimatePacker4
3. Edit your animations as you wish, multiple plists and animations are supported
4. Click `File`-`Svae` to save your animation file (as a `*.xml ` file, it's actually a plist format xml file)
5. Make sure all your `*.plist` and `*.png` and `*.xml` files are all in your game's `res/` folder
6. Import in your game as follow

#### C++

```cpp
...
// load animations into cache
auto ac = AnimationCache::getInstance();
ac->addAnimationsWithFile("Your-awsome-animation-file.xml");

...

// use
auto ac = AnimationCache::getInstance();
auto s = Sprite::create(); // empty sprite
ac->getAnimation("One-of-the-animation-name")->setLoops(-1); // Optional, set loop
auto animate = Animate::create(ac->getAnimation("One-of-the-animation-name"));
s->runAction(animate)
// don't forget to add sprite into your scene.
```

#### Lua

```lua
...
-- load
local ac = cc.AnimationCache:getInstance()
ac:addAnimations("pl00.xml")

...

-- use
local player = cc.Sprite:create()
local animate = cc.Animate:create(ac:getAnimation("player00LeftIdle"))
player:runAction(animate)
player:setPosition(display.center)
player:addTo(self)
```

## 用法

1. 从[release](https://github.com/hellokenlee/AnimatePacker/releases)中下载AnimatePacker4
1. 使用 [TexturePacker](https://www.codeandweb.com/texturepacker) 来打包你的帧动画的所有帧到一个 `.plist`和一个 `.png`文件
2. 拖拉你的`.plist`文件到AnimatePacker4中
3. 编辑你的动画，延迟，名字。 同时支持多个plist混合使用。
4. 点击`File`-`Save`来保存你的动画为一个`.xml`文件(事实上是一个Apple-plist格式的xml)
5. 保证全部你用到过的`.plist`文件和`.png`文件都在你的游戏的`res/`目录或者类似目录下
6. 在代码中导入，使用。 示例代码如上面所示。


## Note

- JS is also supported but I don't write js.
- HUGE shout out to [origin author](https://github.com/gdgoldlion)
