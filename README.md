# lv_font_conv

### 介绍
继lv_img_conv图片转换工具之后，lvgl另一个常用的工具是字体转换工具。仿照先前
lv_img_conv的思路，用C++和FreeType写了一个轻量版的lvgl字体转换工具，
目前也主要是在Linux下命令行的方式使用，因为先前在windows下用的是里飞大佬的
**LvglFontTool**，因此很多地方都是模仿着写的，不过我也加入了一个新功能，详见下文。

### 安装教程
#### 1.安装需要的依赖
这个工具只用到了一个库，就是freetype，用来加载字体位图,我使用的[freetype](https://download.savannah.gnu.org/releases/freetype/)是2.13.0版本，
建议去官网下载同版本安装，避免出现任何编译链接错误。下载**freetype-2.13.0.tar.gz**,然后按照如下步骤安装即可。
```shell
    tar -vxzf freetype-2.13.0.tar.gz
    cd freetype-2.13.0
    ./configure
    make
    sudo make install
```
安装完后/usr/local/lib下会多出freetype的库，在/usr/local/include下会多出一个freetype2的目录，里面存放的就是freetype的头文件。


#### 2.cmake安装
这个工程还是用cmake构建的(其实是用上一个工具lv_conv_img的工程修改而来的)，需要使用cmake，最低版本为3.12.4，如果没有安装或者安装低于
这个版本的建议到[cmake官网](https://cmake.org/files/)下载最新版本安装。


#### 3.工具编译&安装
在上面2步都没问题后，进入源码目录，依次键入以下命令即可安装。
```shell
    mkdir build
    cd build
    cmake ..
    make
    sudo make install
```

### 使用说明
#### 1.帮助信息
使用参数**-h**可以查看帮助信息，包括每个参数的说明。每个参数的输入没有先后顺序要求，但是参数后必须紧跟对应的值，
如-t 后紧跟ttf字库文件，--size后紧跟字高。有些参数有默认值，若未指定该参数则使用默认值。
```shell
    lv_font_conv -h
```
输出的帮助信息
```
lv_font_conv: version 0.1
    -o		指定输出文件名, 同时作为变量名(文件名不包括后缀)
				若不指定该参数则默认为myFont
				此选项还可以指定输出路径,例:"../src/font"(文件名不要加后缀)
	-i		指定输入文件, 存有转换字符的txt文本文档(可选)
	-t		指定TrueType字库文件(.ttf)，此为必要参数
	-c		字体位图输出到C文件(数组), 与"-b"互斥(默认参数)
	-b		字体位图输出为bin文件, 与"-c"互斥
	--size		指定字体尺寸(高度), 单位:像素, 默认值24
	--bpp		指定像素位数(抗锯齿), 可选的有1,2,4(默认值),8
	--ascii		转换前加入ascii字符
	--zHanAll	转换前加入所有汉字
	--zHanCom	转换前加入常用汉字(约6000个)
	--level		指定-b输出到bin文件时, 保存在C文件的字体信息等级, 可选的有0(默认值),1
				数值越大, C文件保存的字体信息越多, 文件IO次数越少,
				渲染速度越快, 但C文件占用空间越大

Tips: -i --ascii --zHanAll --zHanComm四个选项中请至少指定一个选项,
以确保有字符可转换.

```


#### 2.转换示例
**以下示例均使用同一个ttf字库./font/songti.ttf**  
转换基本的ascii字符，字高16, 位数4, 内部字体(字体位图保存为C数组)
```shell
lv_font_conv -t ./font/songti.ttf --ascii --size 16 --bpp 4 -c
```
转换上一级目录下font.txt中的所有字符，字高16，位数4，内部字体(字体位图保存为C数组)，并保存到当前目录下的fonts目录中，
命名为myFont(不加后缀，因为如果是外部字体，还会生成一个bin文件)
```shell
lv_font_conv -t ./font/songti.ttf -i ../font.txt --size 16 --bpp 4 -c -o fonts/myFont
```

转换ascii字符加上常用的汉字(约6000个)，字高20，位数4, 外部字体(字体位图与字体信息保存为bin文件，字体描述保存为c文件，level 0)
```shell
lv_font_conv -t ./font/songti.ttf --ascii --zHanCom --size 20 --bpp 4 -b --level 0
```

转换ascii字符加上常用的汉字(约6000个)，字高20，位数4, 外部字体(字体位图保存为bin文件，字体描述和字体信息保存为c文件，level 1)
```shell
lv_font_conv -t ./font/songti.ttf --ascii --zHanCom --size 20 --bpp 4 -b --level 1
```

这里的**--level**参数是用来指定*当字体位图保存为bin文件时，字体信息保存的位置*。具体细节和分析见下文。

***
### 字体信息等级
&ensp;根据里飞大佬的**LvglFontTool**生成的外部字体bin文件，
读一个字体的位图需要进行**3次文件IO**、一次字体的信息需要进行**2次文件IO**，而且没有缓存，如果有很多字体需要显示，会对帧率产生比较大的影响。
而我将内部字体和外部字体进行结合，即除了字体位图保存到bin文件外，其余均与内部字体无异，这样不仅只需要在读取字体位图时进行1次文件IO，
而且还具备了缓存的功能，如果当前要显示的字体与上一个字体相同，则可以不需要重新读取字体位图数据。  

&ensp;不过这样做会增加编译后的程序固件大小，每一个文字需要占用**14byte**的空间保存文字信息，当然如果字体的位图特别大，这点空间也不算什么了。
但如果你设备的flash空间实在吃紧，也可以使用**level 0**级别，将除了lvgl的字体描述和相关函数保存在C文件，其余均保存在bin文件，这也就是
里飞大佬的**LvglFontTool**的做法了，能最大限度节省flash空间。  

&ensp;当然，如果你的ram够大，将字库直接加载到ram中，读取时采用直接返回首地址+偏移量的方式，是最高效的方法了。


### 更新记录
**2023.08.24-v0.1版本**  
&ensp;第一个版本发布



