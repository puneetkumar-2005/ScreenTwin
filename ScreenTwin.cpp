/* CREATION OF LORD PUNEET        */


#include <QApplication>    // GUI program needs exactly one QApplication object
#include <QWidget>         // base class of every user interface object in Qt : canvas
#include <QVBoxLayout>     // top to bottom organized stack on resize
#include <QHBoxLayout>     // all making cols by dividing horizontally
#include <QListWidget>     // media schedule
#include <QLabel>          // text notification
#include <QPushButton>     // + and other buttons function call
#include <QScreen>         // monitor properties and pos
#include <QGuiApplication> // sys gui info, hdmi
#include <QLineEdit>       // For volume text input
#include <QComboBox>       // For dropdown selection menus
#include <QGraphicsView>   // The container that shows the scene
#include <QGraphicsScene>  // The 2D surface where we place movable items
#include <QFileDialog>     // To open the "Browse" window
#include <QDir>            // To handle file paths
#include <QFileInfo>       // Needed to extract the short name from the full path
#include <QSvgRenderer>    // for rendering .svg files
#include <QMediaPlayer>    // To play video/audio
#include <QAudioOutput>    // To handle sound
#include <QVideoWidget>    // To show video
#include <QGraphicsVideoItem>  // The video screen inside your Graphics Scene
#include <QUrl>                // To handle file paths properly
#include <QGraphicsPixmapItem> //resize handle around media
#include <QGraphicsRectItem>   //display static images (JPEG, PNG, SVG) on the scene
#include <QGraphicsSceneMouseEvent> // Required for drag logic
#include <QSlider>             // For seeking/scrubbing video
#include <QGraphicsSvgItem>    // For displaying SVG vector files
#include <QTimer>              // For delaying the screen refresh slightly
#include <QMenu>               // For the + button popup menu
#include <QInputDialog>        // To show a quick dropdown of open windows
#include <QMediaCaptureSession> // To handle live video feeds
#include <QWindowCapture>      // To grab specific open windows
#include <QCapturableWindow>   // The data object for a window
#include <QMap>                // To track multiple screens and frames
#include <QGraphicsTextItem>   // For the screen name labels
#include <QAudioDecoder>       // The engine that rips audio bytes from the file
#include <QAudioSink>          // The engine that forces bytes into a specific hardware port
#include <QAudioFormat>        // Math definitions for the audio shape (Hz, bit-depth)
#include <QAudioBuffer>        // The bucket that holds the raw 0s and 1s
#include <QMediaDevices>       // To scan for physical sound cards
#include <QAudioDevice>        // Data object for a sound card
#include <QTableWidget>        // For the 3-column grid
#include <QHeaderView>         // To stretch the table columns
#include <QCheckBox>           // For the L/R channel ticks



// --- ROUTING DATA OBJECT ---
struct DeviceRouteState {
    bool outL_inL = false; // Left Speaker plays Source Left
    bool outL_inR = false; // Left Speaker plays Source Right
    bool outR_inL = false; // Right Speaker plays Source Left
    bool outR_inR = false; // Right Speaker plays Source Right
};

// --- HELPER CLASS: Multi-Select Screen Menu ---
class MultiScreenMenu : public QPushButton
{
    Q_OBJECT
public:
    QMenu *menu;
    MultiScreenMenu(QWidget *parent = nullptr) : QPushButton("🖥️ Select Monitors...", parent) {
        menu = new QMenu(this);
        setMenu(menu);
        connect(menu, &QMenu::aboutToShow, this, &MultiScreenMenu::refreshMenu);
    }

signals:
    void screenToggled(QScreen *screen, bool isEnabled);

private slots:
    void refreshMenu() {
        // 1. Remember what is already checked
        QStringList checkedNames;
        for (QAction *act : menu->actions()) {
            if (act->isChecked()) checkedNames << act->text();
        }

        // 2. Clear and rebuild the list from the OS
        menu->clear();
        QList<QScreen*> screens = QGuiApplication::screens();

        for (int i = 0; i < screens.size(); ++i) {
            QScreen *scr = screens.at(i);
            QString screenName = QString("Display %1: %2").arg(i + 1).arg(scr->name());
            
            QAction *action = new QAction(screenName, this);
            action->setCheckable(true);
            
            // Re-check it if it was already active
            if (checkedNames.contains(screenName)) action->setChecked(true);

            // THE FIX: Explicitly bind the pointer to the Lambda so it survives the loop!
            connect(action, &QAction::toggled, this, [this, scr](bool checked){
                emit screenToggled(scr, checked);
            });
            
            menu->addAction(action);
        }
    }
};

// --- HELPER CLASS: The Manual Virtual Camera Frame ---
class VirtualCamera : public QObject, public QGraphicsRectItem {
    Q_OBJECT
public:
    QString screenId;
    QGraphicsView *laptopView; 
    
    int interactionMode = 0; // 0:None, 1:Move, 3:Rotate, 4:Right, 5:Bottom, 6:Left, 7:Top
    QPointF clickPos;
    qreal startRotation;

    VirtualCamera(QString name, QGraphicsView *mainView, QGraphicsItem *parent = nullptr) 
        : QObject(), QGraphicsRectItem(parent) {
        
        screenId = name;
        laptopView = mainView; 
        
        // Start massive (1920x1080)
        setRect(0, 0, 1920, 1080);
        setTransformOriginPoint(rect().center()); 
        
        setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
        setAcceptHoverEvents(true); 
    }

    // --- EXPAND BOUNDARY FOR ROTATION HANDLE ---
    QRectF boundingRect() const override {
        return QGraphicsRectItem::boundingRect().adjusted(-30, -80, 30, 80); 
    }
    QPainterPath shape() const override {
        QPainterPath path;
        path.addRect(boundingRect()); 
        return path;
    }

    // --- DRAW UI HANDLES ---
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override {
        Q_UNUSED(option);
        if (laptopView && widget != laptopView->viewport()) return; 

        painter->setPen(QPen(Qt::green, 6));
        painter->setBrush(QColor(0, 255, 0, 15)); 
        painter->drawRect(rect());

        painter->setPen(Qt::green);
        painter->setFont(QFont("Arial", 24, QFont::Bold));
        painter->drawText(QRectF(0, rect().bottom() + 10, 1000, 40), Qt::AlignLeft, screenId);
        
        if (isSelected()) {
            painter->setBrush(Qt::yellow);
            painter->setPen(QPen(Qt::black, 2));
            painter->drawEllipse(rect().center().x() - 15, rect().top() - 40, 30, 30); 
            painter->drawLine(rect().center().x(), rect().top(), rect().center().x(), rect().top() - 10); 
        }
    }

    // --- EDGE HOVER DETECTION ---
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override {
        if (!isSelected()) {
            setCursor(Qt::ArrowCursor);
            QGraphicsRectItem::hoverMoveEvent(event);
            return;
        }
        QPointF p = event->pos();
        QRectF r = rect();
        qreal m = 15.0; // Grab zone
        
        QRectF rotHandle(r.center().x() - 15, r.top() - 40, 30, 30);

        if (rotHandle.contains(p)) setCursor(Qt::OpenHandCursor); 
        else if (p.x() >= r.right() - m && p.x() <= r.right() + m) setCursor(Qt::SizeHorCursor); 
        else if (p.x() <= r.left() + m && p.x() >= r.left() - m) setCursor(Qt::SizeHorCursor); 
        else if (p.y() >= r.bottom() - m && p.y() <= r.bottom() + m) setCursor(Qt::SizeVerCursor); 
        else if (p.y() <= r.top() + m && p.y() >= r.top() - m) setCursor(Qt::SizeVerCursor); 
        else setCursor(Qt::SizeAllCursor); 
        
        QGraphicsRectItem::hoverMoveEvent(event);
    }

    // --- PURE MANUAL DRAG LOGIC ---
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override {
        QPointF p = event->pos();
        QRectF r = rect();
        qreal m = 15.0;
        clickPos = event->scenePos();
        startRotation = rotation();
        
        QRectF rotHandle(r.center().x() - 15, r.top() - 40, 30, 30);

        if (isSelected() && rotHandle.contains(p)) interactionMode = 3; 
        else if (isSelected() && p.x() >= r.right() - m && p.x() <= r.right() + m) interactionMode = 4; // Right
        else if (isSelected() && p.x() <= r.left() + m && p.x() >= r.left() - m) interactionMode = 6;  // Left
        else if (isSelected() && p.y() >= r.bottom() - m && p.y() <= r.bottom() + m) interactionMode = 5; // Bottom
        else if (isSelected() && p.y() <= r.top() + m && p.y() >= r.top() - m) interactionMode = 7;   // Top
        else interactionMode = 1; // Move
        
        QGraphicsRectItem::mousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override {
        QPointF localPos = mapFromScene(event->scenePos());
        QRectF r = rect();

        if (interactionMode == 1) { // MOVE
            QPointF diff = event->scenePos() - clickPos;
            setPos(pos() + diff);
            clickPos = event->scenePos();
            emit cameraChanged();
        } 
        else if (interactionMode == 3) { // ROTATE
            QPointF centerScene = mapToScene(transformOriginPoint());
            double angle1 = qAtan2(clickPos.y() - centerScene.y(), clickPos.x() - centerScene.x());
            double angle2 = qAtan2(event->scenePos().y() - centerScene.y(), event->scenePos().x() - centerScene.x());
            setRotation(startRotation + ((angle2 - angle1) * 180.0 / M_PI));
            emit cameraChanged();
        }
        else if (interactionMode >= 4 && interactionMode <= 7) { 
            // 1. Record the exact center of the box BEFORE we resize it
            QPointF oldCenterScene = mapToScene(r.center());

            // 2. Perform the blind resize
            if (interactionMode == 4 && localPos.x() > r.left() + 100) r.setRight(localPos.x());
            else if (interactionMode == 5 && localPos.y() > r.top() + 100) r.setBottom(localPos.y());
            else if (interactionMode == 6 && localPos.x() < r.right() - 100) r.setLeft(localPos.x());
            else if (interactionMode == 7 && localPos.y() < r.bottom() - 100) r.setTop(localPos.y());

            // 3. Update the rectangle and tell Qt where the new rotation anchor is
            QGraphicsRectItem::setRect(r);
            setTransformOriginPoint(r.center());

            // 4. Record the new center, and physically shift the whole box backward to cancel out the pendulum swing!
            QPointF newCenterScene = mapToScene(r.center());
            setPos(pos() + (oldCenterScene - newCenterScene));

            emit cameraChanged();
        }
        else {
            QGraphicsRectItem::mouseMoveEvent(event);
        }
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override {
        interactionMode = 0;
        QGraphicsRectItem::mouseReleaseEvent(event);
    }

signals:
    void cameraChanged();
};

// --- HELPER CLASS: The True WYSIWYG Projector Window ---
class ProjectorView : public QGraphicsView {
public:
    ProjectorView(QGraphicsScene *scene) : QGraphicsView(scene) {
        setStyleSheet("background-color: #000; border: none;");
        // Force the background to be black. This is what shows if the Green Box is dragged off the media.
        setBackgroundBrush(Qt::black);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setCursor(Qt::BlankCursor);
        // Crucial: Tell the view to align the scene to the top-left of the viewport before applying transformations.
        setAlignment(Qt::AlignLeft | Qt::AlignTop);
    }
};


// --- HELPER CLASS: Direct Audio Table Popup ---
class MultiAudioMenu : public QPushButton
{
    Q_OBJECT
public:
    QMap<QString, DeviceRouteState> routingState;

    MultiAudioMenu(QWidget *parent = nullptr) : QPushButton("🎛️ Audio Routing Matrix...", parent) {
        connect(this, &QPushButton::clicked, this, &MultiAudioMenu::showPopup);
    }

signals:
    void routingMatrixUpdated(QMap<QString, DeviceRouteState> newState);

private slots:
    void showPopup() {
        QWidget *popup = new QWidget(this, Qt::Popup);
        QVBoxLayout *layout = new QVBoxLayout(popup);
        layout->setContentsMargins(0, 0, 0, 0); 
        
        QTableWidget *table = new QTableWidget(popup);
        table->setColumnCount(3);
        table->setHorizontalHeaderLabels({"Hardware Speaker", "Source L", "Source R"});
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        table->verticalHeader()->hide(); 
        
        QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
        table->setRowCount(devices.size() * 2); // 2 rows per device now!

        for (int i = 0; i < devices.size(); ++i) {
            QString devName = devices.at(i).description();
            if (!routingState.contains(devName)) routingState[devName] = DeviceRouteState();
            
            int rowL = i * 2;
            int rowR = i * 2 + 1;

            // --- 1. LEFT SPEAKER ROW ---
            QTableWidgetItem *nameL = new QTableWidgetItem(devName + " (Left Speaker)");
            nameL->setFlags(nameL->flags() & ~Qt::ItemIsEditable);
            nameL->setBackground(QColor(30, 30, 30)); // Slight alternating color
            table->setItem(rowL, 0, nameL);
            
            QCheckBox *cb_LL = new QCheckBox(); cb_LL->setChecked(routingState[devName].outL_inL);
            connect(cb_LL, &QCheckBox::toggled, this, [this, devName](bool c){ routingState[devName].outL_inL = c; emit routingMatrixUpdated(routingState); });
            QWidget *w_LL = new QWidget(); QHBoxLayout *l_LL = new QHBoxLayout(w_LL); l_LL->addWidget(cb_LL); l_LL->setAlignment(Qt::AlignCenter); l_LL->setContentsMargins(0,0,0,0);
            table->setCellWidget(rowL, 1, w_LL);

            QCheckBox *cb_LR = new QCheckBox(); cb_LR->setChecked(routingState[devName].outL_inR);
            connect(cb_LR, &QCheckBox::toggled, this, [this, devName](bool c){ routingState[devName].outL_inR = c; emit routingMatrixUpdated(routingState); });
            QWidget *w_LR = new QWidget(); QHBoxLayout *l_LR = new QHBoxLayout(w_LR); l_LR->addWidget(cb_LR); l_LR->setAlignment(Qt::AlignCenter); l_LR->setContentsMargins(0,0,0,0);
            table->setCellWidget(rowL, 2, w_LR);

            // --- 2. RIGHT SPEAKER ROW ---
            QTableWidgetItem *nameR = new QTableWidgetItem(devName + " (Right Speaker)");
            nameR->setFlags(nameR->flags() & ~Qt::ItemIsEditable);
            table->setItem(rowR, 0, nameR);

            QCheckBox *cb_RL = new QCheckBox(); cb_RL->setChecked(routingState[devName].outR_inL);
            connect(cb_RL, &QCheckBox::toggled, this, [this, devName](bool c){ routingState[devName].outR_inL = c; emit routingMatrixUpdated(routingState); });
            QWidget *w_RL = new QWidget(); QHBoxLayout *l_RL = new QHBoxLayout(w_RL); l_RL->addWidget(cb_RL); l_RL->setAlignment(Qt::AlignCenter); l_RL->setContentsMargins(0,0,0,0);
            table->setCellWidget(rowR, 1, w_RL);

            QCheckBox *cb_RR = new QCheckBox(); cb_RR->setChecked(routingState[devName].outR_inR);
            connect(cb_RR, &QCheckBox::toggled, this, [this, devName](bool c){ routingState[devName].outR_inR = c; emit routingMatrixUpdated(routingState); });
            QWidget *w_RR = new QWidget(); QHBoxLayout *l_RR = new QHBoxLayout(w_RR); l_RR->addWidget(cb_RR); l_RR->setAlignment(Qt::AlignCenter); l_RR->setContentsMargins(0,0,0,0);
            table->setCellWidget(rowR, 2, w_RR);
        }
        
        layout->addWidget(table);
        popup->resize(550, 350); // Made slightly bigger to hold the extra rows
        QPoint globalPos = mapToGlobal(QPoint(0, -popup->height()));
        popup->move(globalPos);
        popup->show();
    }
};







class ScreenTwin : public QWidget       // new custom window
{
    Q_OBJECT                            // realtime superpowers

public:
    ScreenTwin(QWidget *parent = nullptr) : QWidget(parent)     // startup constructor
    {  
    
        // --- DEFINE GLOBAL AUDIO FORMAT ---
        audioFormat.setSampleRate(48000);
        audioFormat.setChannelCount(2); 
        audioFormat.setSampleFormat(QAudioFormat::Int16);
        
        // --- 1. Master Layout (The vertical "God" container) ---
        // This stacks the Top Bar and the Middle Area vertically.
        QVBoxLayout *masterLayout = new QVBoxLayout(this);

        // --- 2. Top Bar (Spans Full Width) ---
        QHBoxLayout *topBarLayout = new QHBoxLayout();
        masterLayout->addLayout(topBarLayout);


    //Create the labels
        QLabel *appNameLabel = new QLabel("ScreenTwin");
        
        QLabel *screenLabel = new QLabel("Project To:");
        // --- MULTI-SELECT SCREEN DETECTION ---
        multiScreenBtn = new MultiScreenMenu(this);
        connect(multiScreenBtn, &MultiScreenMenu::screenToggled, this, &ScreenTwin::toggleProjector);
        // --------------------------------
        
        topBarLayout->addWidget(appNameLabel);
        topBarLayout->addStretch(); 
        topBarLayout->addWidget(screenLabel);
        topBarLayout->addWidget(multiScreenBtn);


        // --- 3. Middle Area (Container for the two-column split) ---
        // We give this a stretch factor of 1 so it expands to fill the screen height.
        QHBoxLayout *middleAreaLayout = new QHBoxLayout();
        masterLayout->addLayout(middleAreaLayout, 1);

        // --- 4. Defining the Columns (2/3 Left, 1/3 Right) ---
        QVBoxLayout *leftControlColumn = new QVBoxLayout();
        QVBoxLayout *rightMediaColumn = new QVBoxLayout();

        middleAreaLayout->addLayout(leftControlColumn, 2); // Takes 2/3 width
        middleAreaLayout->addLayout(rightMediaColumn, 1);  // Takes 1/3 width



    //Right Media Column Logic
        // Made this a class member (see private section) so buttons can control it
        mediaList = new QListWidget();
        
        // This ensures the list doesn't wrap icons and keeps items in a clean vertical stack
        mediaList->setFlow(QListWidget::TopToBottom);
        mediaList->setResizeMode(QListWidget::Adjust);

        //Create the 'Default Background' as the first permanent item
        QListWidgetItem *defaultBgItem = new QListWidgetItem(mediaList);
        defaultBgItem->setText("DEFAULT BACKGROUND");
        
        //Setting height to 300px for now. 
        // Logic for "Zoom to Fill" (Cropping) vs "Best Fit" (Letterbox) 
        // will be handled in the custom Painter we write for these items later.
        defaultBgItem->setSizeHint(QSize(0, 300));
        defaultBgItem->setTextAlignment(Qt::AlignCenter);

        //Add the list to the right column layout (1/3 width)
        rightMediaColumn->addWidget(mediaList, 1);
    
    // CONNECTING THE NERVES: Click Item -> Play Video
        connect(mediaList, &QListWidget::itemClicked, this, &ScreenTwin::playMedia);
    
        //The Add Button sits at the very bottom
        QPushButton *addMediaButton = new QPushButton("+");
        addMediaButton->setMinimumHeight(80);
        //run the private 'addMedia' function
        // Show a popup menu instead of going straight to the file browser
        connect(addMediaButton, &QPushButton::clicked, this, &ScreenTwin::showAddMenu);
        rightMediaColumn->addWidget(addMediaButton);




        // --- 5. Internal Sections for the Left Column ---
        // Top part: Control Area (where playback buttons/progress bars will go)
        QVBoxLayout *controlAreaLayout = new QVBoxLayout();
        
        
        
        // --- A. The Preview Canvas (Takes 2/3 of this area) ---
        // 1. Create the View (The Widget you see)
        previewView = new QGraphicsView();
        previewView->setStyleSheet("border: 2px solid #555; background-color: #000;");

        // 2. Create the Scene (The internal 1920x1080 stage)
        scene = new QGraphicsScene(this);
        scene->setSceneRect(0, 0, 1920, 1080); // Enforcing 16:9 Aspect Ratio logic
        previewView->setScene(scene);
    
    // --- VIDEO ENGINE SETUP ---
        player = new QMediaPlayer(this);
        audioOutput = new QAudioOutput(this);
        player->setAudioOutput(audioOutput); // Connect sound
        
        // Create the "Screen" inside the Scene
        videoItem = new QGraphicsVideoItem();
        videoItem->setSize(QSizeF(1920, 1080)); // Full HD Size
        scene->addItem(videoItem); // Put screen on stage
        
        // Link Player -> Screen
        player->setVideoOutput(videoItem);
        
        // --- LIVE CAPTURE SETUP ---
        captureSession = new QMediaCaptureSession(this);
        windowCapture = new QWindowCapture(this);
        captureSession->setWindowCapture(windowCapture);
        // We do NOT set the VideoOutput here yet, because the video player is currently using it!
        // We will swap them dynamically when you play a file vs a live window.
    
    
    // 2. The Image Actor (For JPEG/PNG/SVG)
        imageItem = new QGraphicsPixmapItem();
        scene->addItem(imageItem); // Put image on stage
        
        // Hide it initially (we only show it when playing an image)
        imageItem->hide();
    // 3. The SVG Actor (New!)
        svgItem = new QGraphicsSvgItem();
        scene->addItem(svgItem);
        svgItem->hide();
        // 3. Add to layout with Stretch Factor '2'
        controlAreaLayout->addWidget(previewView, 2);


        // --- B. The Playback Controls (Takes 1/3 of this area) ---
        QVBoxLayout *playbackBottomLayout = new QVBoxLayout();

        // 1. Current File Name (Top of controls)
        nowPlayingLabel = new QLabel("Now Playing: None");
        nowPlayingLabel->setAlignment(Qt::AlignCenter);
        nowPlayingLabel->setStyleSheet("font-weight: bold; color: #ddd; margin-bottom: 5px;");
        playbackBottomLayout->addWidget(nowPlayingLabel);

        // 2. Progress Slider
        seekSlider = new QSlider(Qt::Horizontal);
        seekSlider->setRange(0, 0);
        seekSlider->setFixedHeight(50); // Tighten the invisible bounding box
        
        // --- FORCE THICK UI PROPORTIONS ---
        seekSlider->setStyleSheet(
            "QSlider::groove:horizontal { "
            "   border: 1px solid #555; height: 24px; background: #222; border-radius: 4px; "
            "}"
            "QSlider::sub-page:horizontal { "
            "   background: #2e8b57; border-radius: 4px; " // Green filled portion
            "}"
            "QSlider::handle:horizontal { "
            "   background: #ddd; border: 2px solid #111; width: 34px; margin: -8px 0; border-radius: 4px; " // Massive grab handle
            "}"
        );
        
        playbackBottomLayout->addWidget(seekSlider);

        // Connect the Slider to the Player
        // A. When video plays, move slider:
        connect(player, &QMediaPlayer::positionChanged, this, [&](qint64 pos){
            if (!seekSlider->isSliderDown()) seekSlider->setValue(pos);
        });
        // B. When video loads, set slider range:
        connect(player, &QMediaPlayer::durationChanged, this, [&](qint64 dur){
            seekSlider->setRange(0, dur);
        });
        // C. When USER drags slider, seek video AND OUR CUSTOM AUDIO ENGINE:
        connect(seekSlider, &QSlider::sliderMoved, this, [&](int pos){
            player->setPosition(pos);
            // Math: (Milliseconds / 1000) * 48000 Hz * 2 bytes per sample
            currentAudioReadPos = (pos / 1000.0) * 48000 * 2;
            if (currentAudioReadPos % 2 != 0) currentAudioReadPos--; // Ensure byte alignment
        });

        // --- START THE REAL-TIME DSP FEEDER CLOCK ---
        audioTimer = new QTimer(this);
        connect(audioTimer, &QTimer::timeout, this, &ScreenTwin::feedHardwareSinks);
        audioTimer->start(10); // Wakes up every 10 milliseconds to feed the monitors

        // --- ROW 1: NAVIGATION ---
        QHBoxLayout *row1Layout = new QHBoxLayout();
        QPushButton *prevBtn = new QPushButton("<< Prev");
        QPushButton *playBtn = new QPushButton("Play / Pause");
        QPushButton *nextBtn = new QPushButton("Next >>");

        prevBtn->setMinimumHeight(40);
        playBtn->setMinimumHeight(40);
        nextBtn->setMinimumHeight(40);

        row1Layout->addWidget(prevBtn); 
        row1Layout->addWidget(playBtn); 
        row1Layout->addWidget(nextBtn); 

        // --- ROW 2: SETTINGS & ROUTING ---
        QHBoxLayout *row2Layout = new QHBoxLayout();
        loopBtn = new QPushButton("Loop: OFF");
        loopBtn->setMinimumHeight(40);
        
        QLabel *speedLabel = new QLabel("Speed:");
        QLineEdit *speedInput = new QLineEdit("1.0");
        speedInput->setFixedWidth(40); // Small box for numbers like 1.5
        QLabel *speedUnit = new QLabel("x");
        
        // The Pro Audio Matrix Button
        MultiAudioMenu *audioMenuBtn = new MultiAudioMenu(this);
        audioMenuBtn->setMinimumHeight(40);

        // Add to Row 2 with stretch spacers to push things apart cleanly
        row2Layout->addWidget(loopBtn);
        row2Layout->addStretch();
        row2Layout->addWidget(speedLabel);
        row2Layout->addWidget(speedInput);
        row2Layout->addWidget(speedUnit);
        row2Layout->addStretch();
        row2Layout->addWidget(audioMenuBtn);

        // --- CONNECTING THE BUTTONS ---
        connect(playBtn, &QPushButton::clicked, this, &ScreenTwin::togglePlayback);
        connect(nextBtn, &QPushButton::clicked, this, &ScreenTwin::nextMedia);
        connect(prevBtn, &QPushButton::clicked, this, &ScreenTwin::prevMedia);
        connect(loopBtn, &QPushButton::clicked, this, &ScreenTwin::toggleLoop);
        connect(audioMenuBtn, &MultiAudioMenu::routingMatrixUpdated, this, &ScreenTwin::updateDSPRoutes);

        // LOGIC: Press Enter in the box to change Speed
        connect(speedInput, &QLineEdit::returnPressed, [=](){
            double rate = speedInput->text().toDouble();
            if(rate > 0) player->setPlaybackRate(rate);
        });
        
        // Assemble the final playback block
        playbackBottomLayout->addLayout(row1Layout);
        playbackBottomLayout->addSpacing(5); // Add a tiny gap between the rows
        playbackBottomLayout->addLayout(row2Layout);

        // Add this entire cluster to the main Control Layout
        controlAreaLayout->addLayout(playbackBottomLayout, 1);
        
        // The left column now only contains the Video Canvas and our new 2-Row control block!
        leftControlColumn->addLayout(controlAreaLayout, 1);
        
        // AUTO-NEXT LOGIC
        connect(player, &QMediaPlayer::mediaStatusChanged, this, [&](QMediaPlayer::MediaStatus status){
            if(status == QMediaPlayer::EndOfMedia) {
                if(isLooping) player->play();
                else nextMedia();
            }
        });
        
    } // End of Constructor
    
    
    // --- RESIZE EVENT (Best Fit Logic) ---
protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        if(previewView && scene)
            previewView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
        
    }
    
    
private:
    // --- UI ELEMENTS (Needed for resizing) ---
    QListWidget *mediaList; // Needs to be accessed by buttons
    QGraphicsView *previewView;
    QGraphicsScene *scene;
    QLabel *nowPlayingLabel;
    QPushButton *loopBtn;
    QSlider *seekSlider; // Replaces the static Progress Bar
    MultiScreenMenu *multiScreenBtn; // The new checklist button
    // --- MEDIA ENGINE VARIABLES ---
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    QGraphicsVideoItem *videoItem; 
    QGraphicsSvgItem *svgItem;      // Dedicated actor for SVG files
    QGraphicsPixmapItem *imageItem; // For Images (JPEG, PNG, SVG)
    // --- LIVE CAPTURE ENGINE ---
    QMediaCaptureSession *captureSession;
    QWindowCapture *windowCapture;
    // --- MULTI-PROJECTOR TRACKING ---
    struct ProjectorSetup {
        QGraphicsView *view;
        VirtualCamera *frame;
    };
    QMap<QString, ProjectorSetup> activeProjectors;
    // --- STATE ---
    bool isLooping = false;
    
    // --- PRO AUDIO DSP ENGINE ---
    QAudioDecoder *audioDecoder = nullptr;
    QAudioFormat audioFormat;
   struct AudioHardwareSink {
        QAudioDevice device;
        QAudioSink *sink;
        QIODevice *buffer;
        DeviceRouteState routes; // Holds the 4 crosspoint audio variables!
    };
    QList<AudioHardwareSink> activeSinks;
    
    
    // --- DSP MEMORY & SYNC CLOCK ---
    QByteArray masterLeft;
    QByteArray masterRight;
    qint64 currentAudioReadPos = 0;
    QTimer *audioTimer;
    
private slots: 
    // This function is PRIVATE because only this class needs to use it.
    
    
    

    // 1. THE DECODER: Unzips the file into RAM in the background (No Freezing)
    void processAudioBuffer() {
        if (!audioDecoder) return;

        while (audioDecoder->bufferAvailable()) {
            QAudioBuffer buffer = audioDecoder->read();
            if (buffer.format().sampleFormat() != QAudioFormat::Int16) continue;
            
            const char *data = buffer.constData<char>();
            int byteCount = buffer.byteCount();
            
            // Stereo Int16 means [Left Low, Left High, Right Low, Right High] -> 4 bytes per frame
            for (int i = 0; i < byteCount; i += 4) {
                masterLeft.append(data + i, 2);      // Save 2 bytes to Left RAM
                masterRight.append(data + i + 2, 2); // Save 2 bytes to Right RAM
            }
        }
    }

    // --- DSP ROUTING ENGINE ---
    void updateDSPRoutes(QMap<QString, DeviceRouteState> matrix) {
        for (auto &sinkData : activeSinks) {
            sinkData.sink->stop();
            delete sinkData.sink;
        }
        activeSinks.clear();

        QList<QAudioDevice> osDevices = QMediaDevices::audioOutputs();
        for (const QAudioDevice &dev : osDevices) {
            QString name = dev.description();
            if (matrix.contains(name)) {
                DeviceRouteState st = matrix[name];
                // If they checked ANY box for this device, open the hardware port
                if (st.outL_inL || st.outL_inR || st.outR_inL || st.outR_inR) {
                    QAudioSink *newSink = new QAudioSink(dev, audioFormat, this);
                    QIODevice *newBuffer = newSink->start();
                    activeSinks.append({dev, newSink, newBuffer, st});
                }
            }
        }
    }

    // 2. THE HARDWARE FEEDER (WITH CROSSPOINT MIXING)
    void feedHardwareSinks() {
        if (player->playbackState() != QMediaPlayer::PlayingState) return;
        if (masterLeft.isEmpty() || activeSinks.isEmpty()) return;

        qint64 minBytesFree = 9999999;
        for (auto &sinkData : activeSinks) {
            qint64 free = sinkData.sink->bytesFree();
            if (free > 0 && free < minBytesFree) minBytesFree = free;
        }

        if (minBytesFree == 9999999 || minBytesFree <= 0) return;

        qint64 framesToRead = minBytesFree / 4; 
        if (framesToRead > 4800) framesToRead = 4800; 

        for (auto &sinkData : activeSinks) {
            QByteArray chunk;
            chunk.reserve(framesToRead * 4);
            
            for (qint64 i = 0; i < framesToRead; ++i) {
                qint64 bytePos = currentAudioReadPos + (i * 2);
                if (bytePos + 1 >= masterLeft.size()) break; 

                // Get raw 16-bit soundwave values from memory
                qint16 l_src = *(const qint16*)(masterLeft.constData() + bytePos);
                qint16 r_src = *(const qint16*)(masterRight.constData() + bytePos);

                qint16 out_l = 0;
                qint16 out_r = 0;

                // --- CROSSPOINT MIXING: LEFT SPEAKER ---
                // If they want both Source L and Source R in the Left speaker, we add them and divide by 2 to prevent audio clipping!
                if (sinkData.routes.outL_inL && sinkData.routes.outL_inR) out_l = (l_src / 2) + (r_src / 2);
                else if (sinkData.routes.outL_inL) out_l = l_src;
                else if (sinkData.routes.outL_inR) out_l = r_src;

                // --- CROSSPOINT MIXING: RIGHT SPEAKER ---
                if (sinkData.routes.outR_inL && sinkData.routes.outR_inR) out_r = (l_src / 2) + (r_src / 2);
                else if (sinkData.routes.outR_inL) out_r = l_src;
                else if (sinkData.routes.outR_inR) out_r = r_src;

                // Push the custom math bytes to the hardware!
                chunk.append((const char*)&out_l, sizeof(qint16));
                chunk.append((const char*)&out_r, sizeof(qint16));
            }
            if (chunk.size() > 0) sinkData.buffer->write(chunk);
        }
        
        currentAudioReadPos += framesToRead * 2;
    }
    
    // --- MULTI-MONITOR ENGINE ---
    void toggleProjector(QScreen *targetScreen, bool enable)
    {
        QString name = targetScreen->name();

        if (enable)
        {
            // 1. SILENT SAFETY CHECK: Ignore ghost monitors
            if (targetScreen->geometry().width() <= 0 || targetScreen->geometry().height() <= 0) return;

            // 2. Create the Virtual Camera Frame (Passing the previewView so it knows where to draw!)
            VirtualCamera *cam = new VirtualCamera(name, previewView);
            scene->addItem(cam);
            
            // AUTO-FOCUS: Deselect all old boxes so only the new one has visible handles
            for (auto setup : activeProjectors) {
                setup.frame->setSelected(false);
            }
            cam->setSelected(true);

            // Center the frame on the 1920x1080 stage
            cam->setPos((1920.0 / 2.0) - (cam->rect().width() / 2.0), 
                        (1080.0 / 2.0) - (cam->rect().height() / 2.0));

            // 3. Create the borderless Projector Window
            QGraphicsView *projView = new QGraphicsView(scene);
            projView->setStyleSheet("background-color: #000; border: none;");
            projView->setBackgroundBrush(Qt::black); // Force pure black for empty space
            
            // THE FIX: Give the projector an infinitely massive boundary so it can follow the Green Box into the black void without hitting a wall!
            projView->setSceneRect(QRectF(-100000, -100000, 200000, 200000));
            
            projView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            projView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            projView->setCursor(Qt::BlankCursor); 
            
            projView->setGeometry(targetScreen->geometry());
            projView->showFullScreen();

            // 4. BRUTE-FORCE 1:1 MAPPING
            connect(cam, &VirtualCamera::cameraChanged, this, [projView, cam, targetScreen](){
                projView->resetTransform(); 
                
                // A. INVERT the rotation.
                projView->rotate(-cam->rotation()); 
                
                // B. Blindly stretch the frame's exact width and height to match the TV's pixels
                double tvW = targetScreen->size().width();
                double tvH = targetScreen->size().height();
                double camW = cam->rect().width();
                double camH = cam->rect().height();
                projView->scale(tvW / camW, tvH / camH);
                
                // C. Nail the projector exactly to the center of the Green Box
                projView->centerOn(cam->mapToScene(cam->rect().center()));
            });
            emit cam->cameraChanged();
            activeProjectors[name] = {projView, cam};
        }
        else
        {
            // TURN OFF
            if (activeProjectors.contains(name))
            {
                ProjectorSetup setup = activeProjectors[name];
                
                // Destroy the Window on the TV
                setup.view->close();
                delete setup.view;
                
                // Destroy the Green Box on the laptop
                scene->removeItem(setup.frame);
                delete setup.frame;
                
                activeProjectors.remove(name);
            }
        }
    }
    
    // --- ADD MEDIA LOGIC ---

    void showAddMenu() {
        QMenu menu(this);
        QAction *fileAction = menu.addAction("Add File (Video/Image/SVG)");
        QAction *windowAction = menu.addAction("Add Live Window Capture");

        // Show menu exactly where the mouse clicked
        QAction *selected = menu.exec(QCursor::pos());

        if (selected == fileAction) addFile();
        else if (selected == windowAction) addWindow();
    }

    void addFile() {
        QListWidget *list = this->findChild<QListWidget*>();
        if (!list) return;

        QString fileName = QFileDialog::getOpenFileName(this, "Select Media", QDir::homePath(), 
            "Media Files (*.mp4 *.mkv *.mp3 *.wav *.png *.jpg *.jpeg *.svg)");

        if(fileName.isEmpty()) return;
        
        QFileInfo fileInfo(fileName);
        bool isImage = (fileInfo.suffix().toLower() == "png" || fileInfo.suffix().toLower() == "jpg" || fileInfo.suffix().toLower() == "jpeg" || fileInfo.suffix().toLower() == "svg");

        if (list->count() == 1 && list->item(0)->text() == "DEFAULT BACKGROUND" && isImage) {
            QListWidgetItem *placeholder = list->item(0);
            placeholder->setText(fileInfo.fileName()); 
            placeholder->setData(Qt::UserRole, fileName); 
        } else {
            QListWidgetItem *newItem = new QListWidgetItem(list);
            newItem->setText(fileInfo.fileName());
            newItem->setData(Qt::UserRole, fileName);
            newItem->setSizeHint(QSize(0, 300)); 
            newItem->setTextAlignment(Qt::AlignCenter);
        }
    }

    void addWindow() {
        QListWidget *list = this->findChild<QListWidget*>();
        if (!list) return;

        // 1. Ask the OS for all open windows
        QList<QCapturableWindow> windows = QWindowCapture::capturableWindows();
        QStringList windowNames;
        
        for (const QCapturableWindow &win : windows) {
            if (win.isValid() && !win.description().isEmpty()) {
                windowNames << win.description();
            }
        }

        if (windowNames.isEmpty()) {
            nowPlayingLabel->setText("Error: No capturable windows found!");
            return;
        }

        // 2. Show a quick dropdown dialog to the user
        bool ok;
        QString choice = QInputDialog::getItem(this, "Select Window", "Choose a window to capture live:", windowNames, 0, false, &ok);
        
        if (ok && !choice.isEmpty()) {
            // 3. Add to playlist queue! We use a special prefix "window:" to identify it later.
            QListWidgetItem *newItem = new QListWidgetItem(list);
            newItem->setText("[Live] " + choice);
            newItem->setData(Qt::UserRole, "window:" + choice);
            newItem->setSizeHint(QSize(0, 300)); 
            newItem->setTextAlignment(Qt::AlignCenter);
        }
    }
    
    
    
    void playMedia(QListWidgetItem *item)
    {
        if(!item) return;
        QString filePath = item->data(Qt::UserRole).toString();
        
        // 1. Handle Default/Empty
        if (item->text() == "DEFAULT BACKGROUND" && filePath.isEmpty()) {
            player->stop();
            windowCapture->stop(); // Stop live capture
            videoItem->hide();
            imageItem->hide();
            if (svgItem) svgItem->hide(); 
            return;
        }

        // 2. Reset all actors
        player->stop();
        windowCapture->stop(); 
        videoItem->hide();
        imageItem->hide();
        if (svgItem) svgItem->hide();

        // 3. PLAYBACK ROUTING
        if (filePath.startsWith("window:")) {
            
            // --- LIVE WINDOW CAPTURE MODE ---
            QString targetDesc = filePath.mid(7); // Remove the "window:" prefix
            QList<QCapturableWindow> windows = QWindowCapture::capturableWindows();
            QCapturableWindow targetWin;
            
            // Search the OS for the window
            for (const QCapturableWindow &win : windows) {
                if (win.description() == targetDesc) {
                    targetWin = win;
                    break;
                }
            }

            if (targetWin.isValid()) {
                videoItem->show();
                
                // Disconnect Video Player, Connect Live Capture Pipeline
                player->setVideoOutput(nullptr); 
                captureSession->setVideoOutput(videoItem); 
                
                windowCapture->setWindow(targetWin);
                windowCapture->start();
                
                videoItem->setSize(QSizeF(1920, 1080));
                videoItem->setPos(0,0);
            } else {
                nowPlayingLabel->setText("Error: Window was closed or cannot be found!");
                return;
            }

        } else {
            // --- FILE MODE (Video / Image / SVG) ---
            QFileInfo fileInfo(filePath);
            QString ext = fileInfo.suffix().toLower();

            if (ext == "svg") {
                QSvgRenderer *renderer = new QSvgRenderer(filePath);
                if (!renderer->isValid()) {
                    nowPlayingLabel->setText("ERROR: Missing Qt6 SVG Plugin or Corrupt File!");
                    delete renderer;
                    return; 
                }
                svgItem->setSharedRenderer(renderer);
                svgItem->show();
                QRectF rect = svgItem->boundingRect();
                if(rect.height() > 0) {
                     double scale = 1080.0 / rect.height();
                     svgItem->setScale(scale);
                     double newWidth = rect.width() * scale;
                     svgItem->setPos((1920 - newWidth) / 2.0, 0); 
                }

            } else if (ext == "png" || ext == "jpg" || ext == "jpeg") {
                imageItem->setPixmap(QPixmap(filePath));
                imageItem->show();
                imageItem->setPos(0,0);
                imageItem->setScale(1.0); 

            } else {
                // --- VIDEO MODE ---
                videoItem->show();
                
                // Disconnect Live Capture Pipeline, Reconnect Video Player
                captureSession->setVideoOutput(nullptr); 
                player->setVideoOutput(videoItem); 
                
                // --- 1. THE VIDEO BRAIN ---
                player->setSource(QUrl::fromLocalFile(filePath));
                audioOutput->setMuted(true); // SILENCE THE MAIN PLAYER! We will handle the sound.
                videoItem->setSize(QSizeF(1920, 1080));
                videoItem->setPos(0,0);
                player->play();

                // --- 2. THE AUDIO BRAIN (DSP DECODER) ---
                if (audioDecoder) { audioDecoder->stop(); delete audioDecoder; }
                masterLeft.clear();
                masterRight.clear();
                currentAudioReadPos = 0;
                
                audioDecoder = new QAudioDecoder(this);
                audioDecoder->setAudioFormat(audioFormat);
                audioDecoder->setSource(QUrl::fromLocalFile(filePath));
                
                connect(audioDecoder, &QAudioDecoder::bufferReady, this, &ScreenTwin::processAudioBuffer);
                audioDecoder->start(); // Begin ripping the audio!
            }
        }
        // 3. Update Label
        nowPlayingLabel->setText("Now Playing: " + item->text());
        
        // 4. Best Fit Force
        previewView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
        
        // 5. Force ALL active projectors to recalculate their views based on their Virtual Cameras
        for (auto setup : activeProjectors) {
            emit setup.frame->cameraChanged();
        }
        
    }
    
    // --- BUTTON FUNCTIONS ---
    
    void togglePlayback()
    {
        if(player->playbackState() == QMediaPlayer::PlayingState) {
            player->pause();
            for(auto &sd : activeSinks) sd.sink->suspend();
        } else {
            // Re-sync just in case the video drifted while paused
            currentAudioReadPos = (player->position() / 1000.0) * 48000 * 2;
            if (currentAudioReadPos % 2 != 0) currentAudioReadPos--; 
            
            player->play();
            for(auto &sd : activeSinks) sd.sink->resume();
        }
    }
    
    void nextMedia()
    {
        int row = mediaList->currentRow();
        if(row < mediaList->count() - 1)
        {
            mediaList->setCurrentRow(row + 1);
            playMedia(mediaList->currentItem());
        }
    }
    
    void prevMedia()
    {
        int row = mediaList->currentRow();
        if(row > 0)
        {
            mediaList->setCurrentRow(row - 1);
            playMedia(mediaList->currentItem());
        }
    }
    
    void toggleLoop()
    {
        isLooping = !isLooping;
        if(isLooping) loopBtn->setText("Loop: ON");
        else loopBtn->setText("Loop: OFF");
    }

}; // End of Class

// Required for Signals/Slots to work in a single file
#include "screentwin.moc" // Must match your filename "ScreenTwin.cpp"

int main(int argc, char *argv[])
{
    // Force software decoding to bypass buggy Linux VA-API/VDPAU drivers
    qputenv("QT_FFMPEG_HW_DECODE", "0"); 
    qputenv("VDPAU_DRIVER", "none");
    qputenv("LIBVA_DRIVER_NAME", "none");
    
    QApplication app(argc, argv);
    ScreenTwin window;
    
    // Setting a default size
    window.resize(1200, 800); 
    window.setWindowTitle("ScreenTwin");
    window.show();
    
    return app.exec();
}

/*
┌──(kali㉿kali)-[~/Desktop]
└─$ /usr/lib/qt6/libexec/moc ~/Desktop/screentwin.cpp -o ~/Desktop/screentwin.moc
                                                                                                                                                                                                                                           
┌──(kali㉿kali)-[~/Desktop]
└─$ cd ~/Desktop                                                                 
g++ -fPIC screentwin.cpp -o ScreenTwin \
$(pkg-config --cflags --libs Qt6Widgets Qt6Multimedia Qt6MultimediaWidgets Qt6Svg Qt6SvgWidgets)
                                                                                                                                                                                                                                           
┌──(kali㉿kali)-[~/Desktop]
└─$ ./ScreenTwin  

*/
