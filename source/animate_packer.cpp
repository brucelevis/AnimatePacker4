#include <algorithm>

#include <QtDebug>
#include <QtGui>

#include "animate_packer.h"
#include "sprite_frames_list.h"

using namespace std;

AnimatePacker::AnimatePacker(QWidget *parent) :	QMainWindow(parent) {
    ui.setupUi(this);

    QApplication::setWindowIcon(QIcon(":/image/icon.png"));
    setWindowIcon(QIcon(":/image/icon.png"));

    setAcceptDrops(true);

    nullSpriteFramesList = ui.spriteFramesList;
    currentSpriteFramesList = nullSpriteFramesList;
    isPlaying = false;
    zoom = 1;

    connect(ui.plistList, SIGNAL(plistDeleted(QString)), this,SLOT(removePlist(QString)));
    connect(ui.animationTable, SIGNAL(animationSelected(int)), this,SLOT(openSpritesFramesList(int)));
    connect(ui.animationTable, SIGNAL(animationDeleted(int)), this,	SLOT(deleteSpritesFramesList(int)));
    connect(ui.animationTable,SIGNAL(itemChanged ( QTableWidgetItem *  )),this,SLOT(  changeAnimationAttribute(QTableWidgetItem* )));
    connect(ui.spritesList,SIGNAL(currentItemChanged (QListWidgetItem* ,QListWidgetItem*)),this,SLOT(changePreviewSpriteFrame(QListWidgetItem * , QListWidgetItem * )));

    connect(ui.actionNew, SIGNAL(triggered()), this, SLOT(createXml()));
    connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(saveXml()));
    connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(loadXml()));
    connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui.actionPlay, SIGNAL(triggered()), this, SLOT(playAnimation()));
    connect(ui.actionStop, SIGNAL(triggered()), this, SLOT(stopAnimation()));
    connect(ui.actionZoomIn, SIGNAL(triggered()), this, SLOT(zoomIn()));
    connect(ui.actionZoomOut, SIGNAL(triggered()), this, SLOT(zoomOut()));
    connect(ui.actionChoose, SIGNAL(triggered()), this, SLOT(chooseFrame()));
    connect(ui.actionDeletePlist, SIGNAL(triggered()), this,SLOT(deletePlist()));
    connect(ui.actionDeleteAnimation, SIGNAL(triggered()), this,SLOT(deleteAnimation()));
    connect(ui.actionDeleteSpriteFrame, SIGNAL(triggered()), this,SLOT(deleteSpriteFrame()));
    connect(ui.actionMoveDown, SIGNAL(triggered()), this,SLOT(moveDownFrame()));
    connect(ui.actionMoveUp, SIGNAL(triggered()), this, SLOT(moveUpFrame()));
    connect(ui.actionAnimation, SIGNAL(triggered()), this,SLOT(createAnimation()));
    connect(ui.actionOpenPlist,SIGNAL(triggered()),this,SLOT(openPlist()));
    connect(ui.actionPalette,SIGNAL(triggered()),this,SLOT(changeBackground()));
    connect(ui.actionCopy,SIGNAL(triggered()),this,SLOT(copyAnimation()));
    connect(ui.actionIconMode,SIGNAL(triggered()),this,SLOT(changeIconMode()));
    connect(ui.actionListMode,SIGNAL(triggered()),this,SLOT(changeListMode()));

}

AnimatePacker::~AnimatePacker() {

}

#define _____CodeBlockAbout_Drop_OpenFile

void AnimatePacker::dragEnterEvent(QDragEnterEvent *event) {
    if (!event->mimeData()->hasUrls())
        return;

    QFileInfo fileInfo(event->mimeData()->urls().at(0).toString());
    QString suffix=fileInfo.suffix();

    if (suffix == "plist") {
        event->acceptProposedAction();
    } else if (suffix == "xml") {
        event->acceptProposedAction();
    }
}

void AnimatePacker::dropEvent(QDropEvent *event) {
    if (!event->mimeData()->hasUrls())
        return;

    QUrl url=event->mimeData()->urls().at(0);
    QString path=url.toLocalFile();
    QFileInfo fileInfo(url.toString());
    QString suffix=fileInfo.suffix();

    if (suffix== "plist") { //添加plist
        ui.plistList->addItem(fileInfo.fileName());

        addPlist(path);

    } else if (suffix == "xml") { //打开新的xml
        createXml(); //清除

        openXml(path);
    }
}

#define _____CodeBlockAbout_Slot

void AnimatePacker::createXml() {
    ui.plistList->clear();
    ui.animationTable->clearContents(); //删除内容
    ui.animationTable->setRowCount(0); //重置行数
    ui.spriteFramesList->clear();
    ui.spritesList->clear();
    ui.imageLabel->setPixmap(QPixmap());
    path.clear();

    spriteNameToImageMap.clear();

    changeSpriteFramesList(nullSpriteFramesList);
}

void AnimatePacker::loadXml() {
    // 弹窗获取文件路径
    QString path = QFileDialog::getOpenFileName(this, tr("Choose to file"), ".", tr("Xml Files(*.xml)"));
    // 检测
    if (path.isEmpty()) {
        return;
    }
    // 创建新的XML
    createXml(); //清除
    // 读取
    openXml(path);
}

///!TODO: 使用firstChild/lastChild扩展性有问题，最好匹配<key>然后取next
void AnimatePacker::openXml(QString path) {
    // 记录文件路径
    this->path = path;
    // 初始化
    QFile file(path);
    QFileInfo fileInfo(path);
    // 获取xml所在路径
    QString dir = fileInfo.absolutePath();
    // 处理文件无法打开
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::warning(
                    this,
                    QObject::tr("Read failure"),
                    QObject::tr("Cannot read file \n%1\n%2")
                        .arg(file.fileName())
                        .arg(file.errorString())
        );
        return;
    }
    // DOM文件对象
    QDomDocument doc;
    // 处理文件无法被DOM解析
    QString errorStr;
    int errorLine;
    int errorColumn;
    if (!doc.setContent(&file, false, &errorStr, &errorLine, &errorColumn)) {
        QMessageBox::warning(
                    this,
                    QObject::tr("Read failure"),
                    QObject::tr("Syntax error in file \n%1\n%2 row %3 column").arg(errorLine).arg(
                        errorColumn));
        return; //加载文件出错
    }
    // 根字典
    QDomElement rootDict = doc.documentElement().firstChildElement("dict");

    // 处理properties字典
    QDomElement plistArray = rootDict.lastChildElement("dict").firstChildElement("array");
    for (unsigned int i = 0; i < plistArray.childNodes().length(); ++i) {
        // 添加到UI的Plist列表
        QDomElement plistNode = plistArray.childNodes().at(i).toElement();
        ui.plistList->addItem(plistNode.text());
        // 读取plist，添加sprite
        QString plistPath = dir + QDir::separator() + plistNode.text();
        addPlist(plistPath);
    }

    // 处理animations字典
    QDomElement animationsDict = rootDict.firstChildElement("dict");
    // 动画数量
    ui.animationTable->setRowCount(animationsDict.childNodes().length() / 2);
    // 逐个处理
    for (unsigned int i = 0; i < animationsDict.childNodes().length() / 2; i++) {
        // 获取当前动画名字以及对应字典
        QDomElement animation = animationsDict.elementsByTagName("dict").at(i).toElement();
        QDomElement animationName = animation.previousSibling().toElement();
        QString name;
        if(animationName.tagName() == "key")
            name = animationName.text();
        // 显示名字
        qInfo(animationName.text().toStdString().c_str());
        QTableWidgetItem *nameItem = new QTableWidgetItem(name);
        ui.animationTable->setItem(i, 0, nameItem);
        // 显示延迟
        QTableWidgetItem *delayItem = new QTableWidgetItem(animation.firstChildElement("real").text());
        ui.animationTable->setItem(i, 1, delayItem);
        // 显示帧序列
        QDomNodeList framesArray = animation.firstChildElement("array").childNodes();
        // 创建一个新的动画列表
        SpriteFramesList *spriteFrameList = new SpriteFramesList(this);
        spriteFramesLists.push_back(spriteFrameList);
        // 写入每个帧
        for (unsigned int j = 0; j < framesArray.length(); j++) {
            QDomElement frame = framesArray.at(j).toElement();
            QString frameName = frame.text();
            qInfo(frameName.toStdString().c_str());
            QImage image = spriteNameToImageMap[frameName];
            qInfo("%d\n", image.format());
            spriteFrameList->addItem(new QListWidgetItem(QIcon(QPixmap::fromImage(image, Qt::AvoidDither)), frameName));
        }
    }
    // 关闭文件
    file.close();
}

void AnimatePacker::saveXml() {
    // 检查路径
    if (path.isNull()) {
        QString tempPath = QFileDialog::getSaveFileName(this, tr("Choose to preserve location"),
                                                        ".", tr("Xml Files(*.xml)"));

        if (tempPath.isEmpty()) {
            return;
        }

        path = tempPath;
    }
    // 打开文件
    QFile file(path);
    // 处理错误
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(
                    this,
                    QObject::tr("Write failure"),
                    QObject::tr("Cannot write file \n%1\n%2")
                        .arg(file.fileName())
                        .arg(file.errorString())
        );
        return;
    }
    // DOM文件-plist类型
    QDomDocument doc("plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"");
    // XML版本声明
    QDomProcessingInstruction instruction;
    instruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(instruction);
    // Plist
    QDomElement plistsElement = doc.createElement("plist");
    plistsElement.setAttribute("version", "1.0");
    doc.appendChild(plistsElement);
    // 根Dict
    QDomElement rootDict = doc.createElement("dict");
    plistsElement.appendChild(rootDict);
    // key = animations
    QDomElement key = doc.createElement("key");
    key.appendChild(doc.createTextNode("animations"));
    rootDict.appendChild(key);
    // animation Dict
    QDomElement animationsDict = doc.createElement("dict");
    rootDict.appendChild(animationsDict);
    // 所有动画
    for (int i = 0; i < ui.animationTable->rowCount(); i++) {
        // 该动画名字
        key = doc.createElement("key");
        key.appendChild(doc.createTextNode(ui.animationTable->item(i, 0)->text()));
        animationsDict.appendChild(key);
        // 该动画字典
        QDomElement nowDict = doc.createElement("dict");
        animationsDict.appendChild(nowDict);
        // 写入延迟
        key = doc.createElement("key");
        key.appendChild(doc.createTextNode("delay"));
        nowDict.appendChild(key);
        QDomElement delay = doc.createElement("real");
        delay.appendChild(doc.createTextNode(ui.animationTable->item(i, 1)->text()));
        nowDict.appendChild(delay);
        // 写入动画帧
        key = doc.createElement("key");
        key.appendChild(doc.createTextNode("frames"));
        nowDict.appendChild(key);
        QDomElement framesArray = doc.createElement("array");
        nowDict.appendChild(framesArray);
        QListWidget *spriteFramesList = spriteFramesLists[i];
        for (int j = 0; j < spriteFramesList->count(); j++) {
            QDomElement frame = doc.createElement("string");
            frame.appendChild(doc.createTextNode(spriteFramesList->item(j)->text()));
            framesArray.appendChild(frame);
        }
    }
    // 写入属性
    key = doc.createElement("key");
    key.appendChild(doc.createTextNode("properties"));
    rootDict.appendChild(key);
    QDomElement propertiesDict = doc.createElement("dict");
    rootDict.appendChild(propertiesDict);
    // 写入采用的plist文件名
    key = doc.createElement("key");
    key.appendChild(doc.createTextNode("spritesheets"));
    propertiesDict.appendChild(key);
    // 逐个plist写入
    QDomElement plists = doc.createElement("array");
    propertiesDict.appendChild(plists);
    for (int i = 0; i < ui.plistList->count(); i++) {
        QDomElement pl = doc.createElement("string");
        pl.appendChild(doc.createTextNode(ui.plistList->item(i)->text()));
        plists.appendChild(pl);
    }
    // 写入Cocos2dx plist格式执行 (指定为格式1)
    key = doc.createElement("key");
    key.appendChild(doc.createTextNode("format"));
    propertiesDict.appendChild(key);
    key = doc.createElement("integer");
    key.appendChild(doc.createTextNode("1"));
    propertiesDict.appendChild(key);
    // 输出文件
    QTextStream out(&file);
    // tab为4个空格长度
    doc.save(out, 4);
    file.close();
}

void AnimatePacker::openPlist(){
    QString path = QFileDialog::getOpenFileName(this, tr("Choose to file"), ".",
                                                tr("Plist Files(*.plist)"));

    if (path.isEmpty()) {
        return;
    }

    QFileInfo fileInfo(path);
    ui.plistList->addItem(fileInfo.fileName());

    addPlist(path);
}

void AnimatePacker::addPlist(QString path) {
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::warning(
                    this,
                    QObject::tr("Read failure"),
                    QObject::tr("Cannot read file \n%1\n%2").arg(file.fileName()).arg(
                        file.errorString()));
        return; //文件无法打开
    }

    QDomDocument doc;

    QString errorStr;
    int errorLine;
    int errorColumn;

    if (!doc.setContent(&file, false, &errorStr, &errorLine, &errorColumn)) {
        QMessageBox::warning(
                    this,
                    QObject::tr("Read failure"),
                    QObject::tr("Syntax error in file \n%1\n%2 row %3 column").arg(errorLine).arg(
                        errorColumn));
        return; //加载文件出错
    }

    QDomElement root = doc.documentElement();
    QDomNodeList dictNodeList = root.elementsByTagName("dict");

    //判断HD模式
    bool isHD = !(path.indexOf("-hd", 0)==-1);
    int mulHD = isHD?2:1;

    //读metadata////
    QDomNode metaDataNode=findMetadataDict(root);
    map<QString,QString> metaDataDict=parseDict(metaDataNode);

    //读fromat
    int format=metaDataDict["format"].toInt();

    //加载图片
    QString imageFileName;
    switch(format)
    {
    case 0:
    case 1:
    case 2:
        imageFileName=metaDataDict["realTextureFileName"];
        break;
    case 3:
        QDomNode nameNode=metaDataNode.toElement().elementsByTagName("dict").at(0);
        map<QString,QString> nameDict=parseDict(nameNode);
        imageFileName=nameDict["textureFileName"]+nameDict["textureFileExtension"];
        break;
    }
    QFileInfo fileInfo(path);
    QString imagePath = fileInfo.absolutePath() + QDir::separator() + imageFileName;
    QImage source(imagePath);

    //读frames////
    QDomNode framesNode=dictNodeList.at(1);
    QDomNodeList framesNodeList=framesNode.childNodes();

    for(int i=0;i<framesNodeList.length();i+=2)
    {
        //读frame名字
        QString frameName=framesNodeList.at(i).toElement().text();

        //读frame图片
        map<QString,QString> frameDict=parseDict(framesNodeList.at(i+1));

        qreal x,y,w,h;
        qreal sx,sy,sw,sh;
        bool isRotated;

        switch(format)
        {
        case 0:
            w=frameDict["originalWidth"].toInt();
            h=frameDict["originalHeight"].toInt();
            sx=frameDict["x"].toInt();
            sy=frameDict["y"].toInt();
            sw=frameDict["width"].toInt();
            sh=frameDict["height"].toInt();
            x=w/2-sw/2+frameDict["offsetX"].toInt();
            y=h/2-sh/2+frameDict["offsetY"].toInt();
            isRotated=false;
            break;
        case 1:
        {
            QRect frame = strToRect(frameDict["frame"]);
            QPoint offset = strToPoint(frameDict["offset"]);
            QSize sourceSize = strToSize(frameDict["sourceSize"]);

            sx=frame.x();
            sy=frame.y();
            sw=frame.width();
            sh=frame.height();
            w=sourceSize.width();
            h=sourceSize.height();
            x=w/2-sw/2+offset.x();
            y=h/2-sh/2+offset.y();
            isRotated=false;
            break;
        }
        case 2:
        {
            QRect frame = strToRect(frameDict["frame"]);
            QRect sourceColorRect = strToRect(frameDict["sourceColorRect"]);
            QSize sourceSize = strToSize(frameDict["sourceSize"]);

            w=sourceSize.width();
            h=sourceSize.height();
            x=sourceColorRect.x();
            y=sourceColorRect.y();
            sx=frame.x();
            sy=frame.y();
            sw=frame.width();
            sh=frame.height();
            isRotated=strToBool(frameDict["rotated"]);
            if(isRotated)
            {
                swap(sw,sh);
            }
            break;
        }
        case 3:
        {//TODO
            QRect textureRect = strToRect(frameDict["textureRect"]);
            QRect spriteColorRect = strToRect(frameDict["spriteColorRect"]);
            QSize spriteSourceSize = strToSize(frameDict["spriteSourceSize"]);

            w=spriteSourceSize.width();
            h=spriteSourceSize.height();
            x=spriteColorRect.x();
            y=spriteColorRect.y();
            sx=textureRect.x();
            sy=textureRect.y();
            sw=textureRect.width();
            sh=textureRect.height();
            isRotated=strToBool(frameDict["textureRotated"]);
            break;
        }
        }

        QImage image(w,h, QImage::Format_ARGB32);
        // 填充透明背景
        image.fill(qRgba(0, 0, 0, 0));
        QPainter painter(&image);

        painter.translate(x,y);

        if(isRotated)
        {
            painter.translate(sh/2,sw/2);//将原点移动到图片中心
            painter.rotate(-90); //顺时针旋转90度
            painter.translate(-sw/2,-sh/2);//使原点复原
        }

        painter.drawImage(0,0,source, sx,sy,sw,sh);

        spriteNameToImageMap[frameName] = image; //覆盖式插入

        ui.spritesList->addItem(new QListWidgetItem(QIcon(QPixmap::fromImage(image)), frameName));
    }

    file.close();
}

void AnimatePacker::removePlist(QString name) {
    //因为各种原因，我并没有把Plist对应那些Sprite在程序内存中保存，所以移除Plist时，会把该Plist打开，并遍历Sprite删除
    QFileInfo fileInfo(this->path);
    QString path = fileInfo.absolutePath() + QDir::separator() + name; //获取xml所在路径
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::warning(
                    this,
                    QObject::tr("Read failure"),
                    QObject::tr("Cannot read file \n%1\n%2").arg(file.fileName()).arg(
                        file.errorString()));

        return;
    }

    QDomDocument doc;

    QString errorStr;
    int errorLine;
    int errorColumn;

    if (!doc.setContent(&file, false, &errorStr, &errorLine, &errorColumn)) {
        QMessageBox::warning(
                    this,
                    QObject::tr("Read failure"),
                    QObject::tr("Syntax error in file \n%1\n%2 row %3 column").arg(errorLine).arg(
                        errorColumn));

        return;
    }

    QDomElement root = doc.documentElement();

    QDomNodeList dictNodeList = root.elementsByTagName("dict");
    QDomNodeList dictNodeList2 =
            dictNodeList.at(0).toElement().elementsByTagName("dict");
    QDomNodeList keyNodeList =
            dictNodeList2.at(0).toElement().elementsByTagName("key");

    //连带清除
    for (unsigned int i = 0; i < keyNodeList.length(); i += 6) {
        QString key = keyNodeList.at(i).toElement().text();

        QList<QListWidgetItem*> spriteItemList = ui.spritesList->findItems(key,Qt::MatchExactly); //精确查找匹配
        QListWidgetItem *spriteItem = spriteItemList.at(0);
        ui.spritesList->removeItemWidget(spriteItem);
        delete spriteItem;

        for (unsigned int j = 0; j < spriteFramesLists.size(); j++) {
            QListWidget *spriteFramesList = spriteFramesLists[j];
            QList<QListWidgetItem*> spriteFrameItemList =
                    spriteFramesList->findItems(key, Qt::MatchExactly); //精确查找匹配

            if (!spriteFrameItemList.isEmpty()) {
                QListWidgetItem *spriteFrameItem = spriteFrameItemList.at(0);

                spriteFramesList->removeItemWidget(spriteFrameItem);
                delete spriteFrameItem;
            }
        }
    }

    file.close();
}

void AnimatePacker::deletePlist() {
    ui.plistList->deleteItem();
}

void AnimatePacker::moveDownFrame() {
    if (currentSpriteFramesList == nullSpriteFramesList)
        return;

    int row = currentSpriteFramesList->currentRow();

    if (currentSpriteFramesList->swapItem(row, row + 1))
        currentSpriteFramesList->setCurrentRow(row + 1);
}

void AnimatePacker::moveUpFrame() {
    if (currentSpriteFramesList == nullSpriteFramesList)
        return;

    int row = currentSpriteFramesList->currentRow();

    if (currentSpriteFramesList->swapItem(row, row - 1))
        currentSpriteFramesList->setCurrentRow(row - 1);
}

void AnimatePacker::chooseFrame() {
    if (!ui.spritesList->count())
        return;

    if(ui.spritesList->currentRow()==-1)
        return;

    QString itemName = ui.spritesList->currentItem()->text();

    if (itemName.isEmpty())
        return;

    if (currentSpriteFramesList == nullSpriteFramesList)
        return;

    currentSpriteFramesList->addItem(itemName);
}

void AnimatePacker::resetListViewMode(){
    ui.spritesList->reset();
}

void AnimatePacker::changeListMode(){
    ui.spritesList->setViewMode(QListView::ListMode);
}

void AnimatePacker::changeIconMode(){
    ui.spritesList->setViewMode(QListView::IconMode);
}

void AnimatePacker::deleteSpriteFrame() {
    currentSpriteFramesList->deleteItem();
}

#define _____CodeBlockAbout_Aniamtion

void AnimatePacker::createAnimation() {
    int currentRow = ui.animationTable->rowCount();
    ui.animationTable->setRowCount(currentRow + 1);
    spriteFramesLists.push_back(new SpriteFramesList(this));

    ui.animationTable->selectRow(currentRow);
    //填入缺省值
    ui.animationTable->setItem (currentRow, 0, new QTableWidgetItem(QString("untitle")+QString::number(currentRow)));
    ui.animationTable->setItem (currentRow, 1, new QTableWidgetItem("0.3"));
    //更新显示
    changeSpriteFramesList(currentRow);
}

void AnimatePacker::changeAnimationAttribute(QTableWidgetItem* item){
    changePreviewSpriteFrame(currentSpriteFramesList->currentItem(),0);
}

void AnimatePacker::copyAnimation(){
    int rowCount=ui.animationTable->rowCount();

    if(rowCount==0)
        return;

    if(ui.animationTable->currentRow()==-1)
        return;

    ui.animationTable->setRowCount(rowCount + 1);

    int currentRow=ui.animationTable->currentRow();
    SpriteFramesList *sourceSpriteFrameList=spriteFramesLists[currentRow];

    ui.animationTable->setItem (rowCount, 0, new QTableWidgetItem(QString("untitle")+QString::number(rowCount)));
    QString delay=ui.animationTable->item(currentRow,1)->text();
    ui.animationTable->setItem (rowCount, 1, new QTableWidgetItem(delay));

    SpriteFramesList *destSpriteFramesList=new SpriteFramesList(this);
    spriteFramesLists.push_back(destSpriteFramesList);

    for(int i=0;i<sourceSpriteFrameList->count();i++){
        QListWidgetItem *listWidgetItem=sourceSpriteFrameList->item(i);
        destSpriteFramesList->addItem(new QListWidgetItem(listWidgetItem->icon(), listWidgetItem->text()));
    }

    ui.animationTable->selectRow(rowCount);
}

void AnimatePacker::deleteAnimation() {
    ui.animationTable->deleteItem();
}

void AnimatePacker::playAnimation() {
    int spriteFrameCount = currentSpriteFramesList->count();

    if (!isPlaying && spriteFrameCount) {
        isPlaying = true;
        QTableWidgetItem *delayItem = ui.animationTable->item(
                    ui.animationTable->currentRow(), 1);
        int interval = delayItem->text().toDouble() * 1000;
        timerId = startTimer(interval);
    }
}

void AnimatePacker::stopAnimation() {
    if (isPlaying) {
        isPlaying = false;
        killTimer(timerId);
    }
}

void AnimatePacker::timerEvent(QTimerEvent *event) {
    int row = currentSpriteFramesList->currentRow();
    int count = currentSpriteFramesList->count();

    if (++row >= count)
        row = 0;

    currentSpriteFramesList->setCurrentRow(row);
}

void AnimatePacker::zoomIn() {
    zoom++;

    changePreviewSpriteFrame(currentSpriteFramesList->currentItem(), 0);
}

void AnimatePacker::zoomOut() {
    if (--zoom < 1)
        zoom = 1;

    changePreviewSpriteFrame(currentSpriteFramesList->currentItem(), 0);
}

void AnimatePacker::openSpritesFramesList(int id) {
    changeSpriteFramesList(id);
}

void AnimatePacker::deleteSpritesFramesList(int id) {
    vector<SpriteFramesList*>::iterator spriteFramesItr = std::find(
                spriteFramesLists.begin(), spriteFramesLists.end(),
                spriteFramesLists[id]);

    if (spriteFramesItr != spriteFramesLists.end()) {
        changeSpriteFramesList(nullSpriteFramesList);

        QListWidget *spriteFramesList = *spriteFramesItr;
        spriteFramesLists.erase(spriteFramesItr);
        delete spriteFramesList;
    }
}

void AnimatePacker::changePreviewSpriteFrame(QListWidgetItem * current,
                                             QListWidgetItem * previous) {
    if (current) {
        QImage image = spriteNameToImageMap[current->text()];
        QPixmap pixmap=QPixmap::fromImage(image);
        //缩放处理
        pixmap=pixmap.scaled(image.size() * zoom, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        ui.imageLabel->setPixmap(pixmap);
    } else {
        ui.imageLabel->clear();
    }
}

void AnimatePacker::changeSpriteFramesList(int id) {
    ui.verticalLayout_3->removeWidget(currentSpriteFramesList);
    currentSpriteFramesList->setHidden(true);
    disconnect(
                currentSpriteFramesList,
                SIGNAL( currentItemChanged (QListWidgetItem* , QListWidgetItem*)),
                this,
                SLOT(changePreviewSpriteFrame(QListWidgetItem * , QListWidgetItem * )));

    if(id==-1){//防止野指针报错
        currentSpriteFramesList = nullSpriteFramesList;
    }else{
        currentSpriteFramesList = spriteFramesLists[id];
    }
    currentSpriteFramesList->setHidden(false);
    ui.verticalLayout_3->addWidget(currentSpriteFramesList);
    connect(
                currentSpriteFramesList,
                SIGNAL( currentItemChanged (QListWidgetItem* , QListWidgetItem*)),
                this,
                SLOT(changePreviewSpriteFrame(QListWidgetItem * , QListWidgetItem * )));

    if (currentSpriteFramesList->count()) {
        currentSpriteFramesList->setCurrentRow(-1);//取消选择,使下一行能个发出信号,但是可能会触发错误
        currentSpriteFramesList->setCurrentRow(0);

        if (isPlaying) {
            stopAnimation();
            playAnimation();
        }
    }else{
        changePreviewSpriteFrame(0,0);
    }
}

void AnimatePacker::changeSpriteFramesList(SpriteFramesList *listWidget) {
    ui.verticalLayout_3->removeWidget(currentSpriteFramesList);
    currentSpriteFramesList->setHidden(true);

    currentSpriteFramesList = listWidget;
    currentSpriteFramesList->setHidden(false);
    ui.verticalLayout_3->addWidget(currentSpriteFramesList);
}

void AnimatePacker::changeBackground(){
    QColor color = QColorDialog::getColor(Qt::white, this);

    if(!color.isValid())
        return;

    fillBackground(ui.imageLabel,color);
}

#define _____CodeBlockAbout_Other

void AnimatePacker::closeEvent(QCloseEvent *event) {
    if (true) {
        event->accept(); //事件接受
    } else {
        event->ignore(); //事件取消
    }

}

void AnimatePacker::about() {
    QMessageBox::about(
                this,
                tr("About"),
                tr("<h2>AnimatePacker4</h2>"
                  "<p>author: goldlion, KenLee</p>"
                  "<p>email: ken_4000@qq.com</p>"
                  "<p>github: <a href=\"https://github.com/hellokenlee/AnimatePacker\">AnimatePacker4</a></p>")
    );
}

QDomNode AnimatePacker::findMetadataDict(QDomElement root)
{
    QDomNodeList nodeList=root.firstChild().childNodes();

    for(int i=0;i<nodeList.length();i++)
    {
        if(nodeList.at(i).toElement().text()=="metadata")
        {
            return nodeList.at(i+1);
        }
    }
}

std::map<QString, QString> AnimatePacker::parseDict(QDomNode dictElm)
{
    QDomNodeList children=dictElm.childNodes();

    map<QString, QString> keyToValueMap;

    for(int i=0;i<children.length();i+=2)
    {
        QDomElement keyElm=children.at(i).toElement();
        QDomElement valueElm=children.at(i+1).toElement();

        keyToValueMap[keyElm.text()]=
                valueElm.text().isEmpty()?valueElm.nodeName():valueElm.text();

        qDebug()<<keyElm.text()<<","<<(valueElm.text().isEmpty()?valueElm.nodeName():valueElm.text())<<endl;
    }

    return keyToValueMap;
}

std::vector<int> AnimatePacker::parseStrToIntArray(QString s)
{
    std::vector<int> numbers;
    QString temp;
    for(int i=0;i<s.length();i++)
    {
        QChar c=s.at(i);
        if(c.isNumber()||c=='-')
        {
            temp.append(c);
        }
        else
        {
            if(!temp.isEmpty())
            {
                int ii=temp.toInt();
                numbers.push_back(temp.toInt());
            }
            temp.clear();
        }
    }

    return numbers;
}

QRect AnimatePacker::strToRect(QString rectStr) {
    std::vector<int> numbers=parseStrToIntArray(rectStr);

    return QRect(numbers[0], numbers[1], numbers[2], numbers[3]);
}

QPoint AnimatePacker::strToPoint(QString pointStr){
    std::vector<int> numbers=parseStrToIntArray(pointStr);

    return QPoint(numbers[0],numbers[1]);
}

QSize AnimatePacker::strToSize(QString sizeStr){
    std::vector<int> numbers=parseStrToIntArray(sizeStr);

    return QSize(numbers[0],numbers[1]);
}

bool AnimatePacker::strToBool(QString boolStr)
{
    return boolStr=="true";
}

void AnimatePacker::fillBackground(QWidget *widget, QColor color) {
    char styleSheetBuf[128];
    sprintf(styleSheetBuf, "background-color: rgb(%d, %d, %d);", color.red(), color.green(), color.blue());
    QString sheet(styleSheetBuf);
    widget->setStyleSheet(sheet);
}
