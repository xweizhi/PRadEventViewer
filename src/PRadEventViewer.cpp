//============================================================================//
// Main class for PRad Event Viewer, derived from QMainWindow                 //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include "PRadEventViewer.h"

#include "TApplication.h"
#include "TSystem.h"
#include "TFile.h"
#include "TH1.h"
#include "TF1.h"

#include "evioUtil.hxx"
#include "evioFileChannel.hxx"

#include <fstream>
#include <iostream>
#include <iomanip>

#if QT_VERSION >= 0x050000
#include <QtWidgets>
#include <QtConcurrent>
#else
#include <QtGui>
#endif

#include "HyCalModule.h"
#include "HyCalScene.h"
#include "HyCalView.h"
#include "Spectrum.h"
#include "SpectrumSettingPanel.h"
#include "HtmlDelegate.h"
#include "PRadETChannel.h"
#include "PRadHVSystem.h"
#include "ETSettingPanel.h"
#include "PRadEvioParser.h"
#include "PRadHistCanvas.h"
#include "PRadDataHandler.h"
#include "PRadDAQUnit.h"
#include "PRadTDCGroup.h"
#include "PRadLogBox.h"

#include <sys/time.h>

#define cap_value(a, min, max) \
        (((a) >= (max)) ? (max) : ((a) <= (min)) ? (min) : (a))
#define event_index currentEvent - 1

//============================================================================//
// constructor                                                                //
//============================================================================//
PRadEventViewer::PRadEventViewer()
: handler(new PRadDataHandler()), currentEvent(0), etChannel(nullptr), hvSystem(nullptr)
{
    initView();
    setupUI();
}

PRadEventViewer::~PRadEventViewer()
{
    delete hvSystem;
    delete etChannel;
    delete handler;
}

// set up the view for HyCal
void PRadEventViewer::initView()
{
    HyCal = new HyCalScene(this, -800, -800, 1600, 1600);
    HyCal->setBackgroundBrush(QColor(255, 255, 238));

    generateSpectrum();
    generateHyCalModules();

    setupOnlineMode();

    view = new HyCalView;
    view->setScene(HyCal);

    // root timer to process root events
    QTimer *rootTimer = new QTimer(this);
    connect(rootTimer, SIGNAL(timeout()), this, SLOT(handleRootEvents()));
    rootTimer->start(50);
}

// set up the UI
void PRadEventViewer::setupUI()
{
    setWindowTitle(tr("PRad Event Viewer"));
    QDesktopWidget dw;
    double height = dw.height();
    double width = dw.width();
    double scale = (width/height > (16./9.))? 0.8 : 0.8 * ((width/height)/(16/9.));
    view->scale((height*scale)/1440, (height*scale)/1440);
    resize(height*scale*16./9., height*scale);

    createMainMenu();
    createStatusBar();

    createControlPanel();
    createStatusWindow();

    rightPanel = new QSplitter(Qt::Vertical);
    rightPanel->addWidget(statusWindow);
    rightPanel->addWidget(controlPanel);
    rightPanel->setStretchFactor(0,7);
    rightPanel->setStretchFactor(1,2);

    mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(view);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0,1);
    mainSplitter->setStretchFactor(1,1);

    setCentralWidget(mainSplitter);

    fileDialog = new QFileDialog();
}

//============================================================================//
// generate elements                                                          //
//============================================================================//

// create spectrum
void PRadEventViewer::generateSpectrum()
{
    energySpectrum = new Spectrum(40, 1100, 1, 3000);
    energySpectrum->setPos(600, 0);
    HyCal->addItem(energySpectrum);

    specSetting = new SpectrumSettingPanel(this);
    specSetting->ConnectSpectrum(energySpectrum);

    connect(energySpectrum, SIGNAL(spectrumChanged()), this, SLOT(Refresh()));
}

// crate HyCal modules from module list
void PRadEventViewer::generateHyCalModules()
{
    readTDCList();
    readModuleList();
    readSpecialChannels();

    // end of channel/module reading
    buildModuleMap();

    readPedestalData("config/pedestal.dat");

    // Default setting
    selection = HyCal->GetModuleList().at(0);
    annoType = NoAnnotation;
    viewMode = EnergyView;

}

//============================================================================//
// create menu and tool box                                                   //
//============================================================================//

// main menu
void PRadEventViewer::createMainMenu()
{
    // file menu, open, save, quit
    QMenu *fileMenu = new QMenu(tr("&File"));

    openDataAction = fileMenu->addAction(tr("&Open Data File"));
    openDataAction->setShortcuts(QKeySequence::Open);

    QAction *saveHistAction = fileMenu->addAction(tr("&Save Histograms"));
    saveHistAction->setShortcuts(QKeySequence::Save);

    openPedAction = fileMenu->addAction(tr("Load &Pedestal"));
    openPedAction->setShortcuts(QKeySequence::Print);

    QAction *quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcuts(QKeySequence::Quit);

    connect(openDataAction, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(openPedAction, SIGNAL(triggered()), this, SLOT(openPedFile()));
    connect(saveHistAction, SIGNAL(triggered()), this, SLOT(saveHistToFile()));
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    menuBar()->addMenu(fileMenu);

    // online menu, toggle on/off online mode
    QMenu *onlineMenu = new QMenu(tr("Online &Mode"));

    onlineEnAction = onlineMenu->addAction(tr("Start Online Mode"));
    onlineDisAction = onlineMenu->addAction(tr("Stop Online Mode"));
    onlineDisAction->setEnabled(false);

    connect(onlineEnAction, SIGNAL(triggered()), this, SLOT(initOnlineMode()));
    connect(onlineDisAction, SIGNAL(triggered()), this, SLOT(stopOnlineMode()));
    menuBar()->addMenu(onlineMenu);

    // high voltage menu
    QMenu *hvMenu = new QMenu(tr("High Voltage"));
    hvEnableAction = hvMenu->addAction(tr("Connect to HV system"));
    hvMonitorAction = hvMenu->addAction(tr("Start HV Monitor"));
    hvMonitorAction->setEnabled(false);
    hvDisableAction = hvMenu->addAction(tr("Disconnect to HV system"));
    hvDisableAction->setEnabled(false);

    connect(hvEnableAction, SIGNAL(triggered()), this, SLOT(connectHVSystem()));
    connect(hvMonitorAction, SIGNAL(triggered()), this, SLOT(startHVMonitor()));
    connect(hvDisableAction, SIGNAL(triggered()), this, SLOT(disconnectHVSystem()));
    menuBar()->addMenu(hvMenu);

    // tool menu, useful tools
    QMenu *toolMenu = new QMenu(tr("&Tools"));

    QAction *snapShotAction = toolMenu->addAction(tr("Take SnapShot"));
    snapShotAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_S));

    QAction *eraseAction = toolMenu->addAction(tr("Erase Buffer"));
    eraseAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_X));

    connect(snapShotAction, SIGNAL(triggered()), this, SLOT(takeSnapShot()));
    connect(eraseAction, SIGNAL(triggered()), this, SLOT(eraseBufferAction()));
    menuBar()->addMenu(toolMenu);

}

// tool box
void PRadEventViewer::createControlPanel()
{
    eventSpin = new QSpinBox;
    eventSpin->setRange(0, 0);
    eventSpin->setPrefix("Event # ");
    connect(eventSpin, SIGNAL(valueChanged(int)),
            this, SLOT(changeCurrentEvent(int)));

    histTypeBox = new QComboBox();
    histTypeBox->addItem(tr("Module Hist"));
    histTypeBox->addItem(tr("LMS Hist"));
    annoTypeBox = new QComboBox();
    annoTypeBox->addItem(tr("No Annotation"));
    annoTypeBox->addItem(tr("Module ID"));
    annoTypeBox->addItem(tr("DAQ Info"));
    annoTypeBox->addItem(tr("Show TDC Group"));
    viewModeBox = new QComboBox();
    viewModeBox->addItem(tr("Energy View"));
    viewModeBox->addItem(tr("Occupancy View"));
    viewModeBox->addItem(tr("Pedestal View"));
    viewModeBox->addItem(tr("Ped. Sigma View"));
    viewModeBox->addItem(tr("High Voltage View"));

    spectrumSettingButton = new QPushButton("Spectrum Settings");

    eventCntLabel = new QLabel;
    eventCntLabel->setText(tr("No events data loaded."));

    connect(histTypeBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeHistType(int)));
    connect(annoTypeBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeAnnoType(int)));
    connect(viewModeBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeViewMode(int)));
    connect(spectrumSettingButton, SIGNAL(clicked()),
            this, SLOT(changeSpectrumSetting()));

    logBox = new PRadLogBox();

    QGridLayout *layout = new QGridLayout();

    layout->addWidget(eventSpin,             0, 0, 1, 1);
    layout->addWidget(eventCntLabel,         0, 1, 1, 1);
    layout->addWidget(spectrumSettingButton, 0, 2, 1, 1);
    layout->addWidget(histTypeBox,           1, 0, 1, 1);
    layout->addWidget(viewModeBox,           1, 1, 1, 1);
    layout->addWidget(annoTypeBox,           1, 2, 1, 1);
    layout->addWidget(logBox,                2, 0, 3, 3);

    controlPanel = new QWidget(this);
    controlPanel->setLayout(layout);

}

// status bar
void PRadEventViewer::createStatusBar()
{
    lStatusLabel = new QLabel(tr("Please open a data file or use online mode."));
    lStatusLabel->setAlignment(Qt::AlignLeft);
    lStatusLabel->setMinimumSize(lStatusLabel->sizeHint());

    rStatusLabel = new QLabel(tr(""));
    rStatusLabel->setAlignment(Qt::AlignRight);


    statusBar()->addPermanentWidget(lStatusLabel, 1);
    statusBar()->addPermanentWidget(rStatusLabel, 1);
}

// Status window
void PRadEventViewer::createStatusWindow()
{
    statusWindow = new QSplitter(Qt::Vertical);

    // status info part
    setupInfoWindow();
    histCanvas = new PRadHistCanvas(this);
    histCanvas->AddCanvas(0, 0, 38);
    histCanvas->AddCanvas(1, 0, 46);
    histCanvas->AddCanvas(2, 0, 30);

    statusWindow->addWidget(statusInfoWidget);
    statusWindow->addWidget(histCanvas);
}

// status infor window
void PRadEventViewer::setupInfoWindow()
{
    statusInfoWidget = new QTreeWidget;
    statusInfoWidget->setStyleSheet("QTreeWidget {background: #FFFFF8;}");
    statusInfoWidget->setSelectionMode(QAbstractItemView::NoSelection);
    QStringList statusInfoTitle;
    QFont font("arial", 10 , QFont::Bold );

    statusInfoTitle << tr("  Module Property  ") << tr("  Value  ")
                    << tr("  Module Property  ") << tr("  Value  ");
    statusInfoWidget->setHeaderLabels(statusInfoTitle);
    statusInfoWidget->setItemDelegate(new HtmlDelegate());
    statusInfoWidget->setIndentation(0);
    statusInfoWidget->setMaximumHeight(180);

    // add new items to status info
    QStringList statusProperty;
    statusProperty << tr("  Module ID") << tr("  Module Type") << tr("  DAQ Address") << tr("  TDC Group") << tr("  HV Address")
                   << tr("  Pedestal") << tr("  Event No.") << tr("  Energy") << tr("  Fired Count") << tr("  High Voltage");

    for(int i = 0; i < 5; ++i) // row iteration
    {
        statusItem[i] = new QTreeWidgetItem(statusInfoWidget);
        for(int j = 0; j < 4; ++ j) // column iteration
        {
            if(j&1) { // odd column
                statusItem[i]->setFont(j, font);
            } else { // even column
                statusItem[i]->setText(j, statusProperty.at(5*j/2 + i));
            }
            if(!(i&1)) { // even row
                statusItem[i]->setBackgroundColor(j, QColor(255,255,208));
            }
        }
    }

    // Spectial rule to enable html text support for subscript
    statusItem[1]->useHtmlDelegate(1);

    statusInfoWidget->resizeColumnToContents(0);
    statusInfoWidget->resizeColumnToContents(2);

}

//============================================================================//
// read information from configuration files                                  //
//============================================================================//

// read tdc groups from file
void PRadEventViewer::readTDCList()
{
    QFile list("config/tdc_group_list.txt");
    if(!list.open(QFile::ReadOnly | QFile::Text)) {
        std::cout << "WARNING: Missing configuration file \"config/tdc_group_list.txt\""
                  << ", cannot create tdc groups!"
                  << std::endl;
    }

    QTextStream in(&list);
    QString name;
    ChannelAddress addr;

    while (!in.atEnd())
    {
        QString line = in.readLine().simplified();
        if(line.at(0) == '#')
            continue;
        QStringList fields = line.split(QRegExp("\\s+"));
        if(fields.size() == 4) {
            name = fields.takeFirst();
            addr.crate = (unsigned char)fields.takeFirst().toInt();
            addr.slot = (unsigned char)fields.takeFirst().toInt();
            addr.channel = (unsigned char)fields.takeFirst().toInt();
            handler->AddTDCGroup(new PRadTDCGroup(name.toStdString(), addr));
        } else {
            std::cout << "Unrecognized input format in configuration file, skipped one line!"
                      << std::endl;
        }
    }
}

// read module list from file
void PRadEventViewer::readModuleList()
{
    QFile list("config/module_list.txt");
    if(!list.open(QFile::ReadOnly | QFile::Text)) {
        std::cerr << "ERROR: Missing configuration file \"config/module_list.txt\""
                  << ", cannot generate HyCal channels!"
                  << std::endl;
        exit(1);
    }

    QTextStream in(&list);
    QString moduleName;
    ChannelAddress daqAddr;
    QString tdcGroup;
    HyCalModule::HVSetup hvInfo;
    HyCalModule::GeoInfo geometry;

    // some info that is not read from list
    // initialize first
    hvInfo.volt.Vmon = 0;
    hvInfo.volt.Vset = 0;
    hvInfo.volt.ON = false;

    while (!in.atEnd())
    {
        QString line = in.readLine().simplified();
        if(line.at(0) == '#')
            continue;
        QStringList fields = line.split(QRegExp("\\s+"));
        if(fields.size() == 12) {
            moduleName = fields.takeFirst();
            daqAddr.crate = (unsigned char)fields.takeFirst().toInt();
            daqAddr.slot = (unsigned char)fields.takeFirst().toInt();
            daqAddr.channel = (unsigned char)fields.takeFirst().toInt();
            tdcGroup = fields.takeFirst();

            geometry.type = (HyCalModule::ModuleType)fields.takeFirst().toInt();
            geometry.cellSize = fields.takeFirst().toDouble();
            geometry.x = fields.takeFirst().toDouble();
            geometry.y = fields.takeFirst().toDouble();

            hvInfo.config.crate = (unsigned char)fields.takeFirst().toInt();
            hvInfo.config.slot = (unsigned char)fields.takeFirst().toInt();
            hvInfo.config.channel = (unsigned char)fields.takeFirst().toInt();

            HyCalModule* newModule = new HyCalModule(this, moduleName, daqAddr, tdcGroup, hvInfo, geometry);
            HyCal->addModule(newModule);
            handler->RegisterChannel(newModule);
        } else {
            std::cout << "Unrecognized input format in configuration file, skipped one line!"
                      << std::endl;
        }
    }

    list.close();
}

void PRadEventViewer::readSpecialChannels()
{
    QFile list("config/special_channels.txt");
    if(!list.open(QFile::ReadOnly | QFile::Text)) {
        std::cerr << "WARNING: Missing configuration file \"config/special_channels.txt\"."
                  << std::endl;
        return;
    }

    QTextStream in(&list);
    QString moduleName;
    ChannelAddress daqAddr;
    QString tdcGroup;
    HyCalModule::HVSetup hvInfo;

    // some info that is not read from list
    while (!in.atEnd())
    {
        QString line = in.readLine().simplified();
        if(line.at(0) == '#')
            continue;
        QStringList fields = line.split(QRegExp("\\s+"));
        if(fields.size() == 8) {
            moduleName = fields.takeFirst();
            daqAddr.crate = (unsigned char)fields.takeFirst().toInt();
            daqAddr.slot = (unsigned char)fields.takeFirst().toInt();
            daqAddr.channel = (unsigned char)fields.takeFirst().toInt();
            tdcGroup = fields.takeFirst();
/*
            hvInfo.config.crate = (unsigned char)fields.takeFirst().toInt();
            hvInfo.config.slot = (unsigned char)fields.takeFirst().toInt();
            hvInfo.config.channel = (unsigned char)fields.takeFirst().toInt();
*/
            handler->AddChannel(new PRadDAQUnit(moduleName.toStdString(), daqAddr, tdcGroup.toStdString()));
        } else {
            std::cout << "Unrecognized input format in configuration file, skipped one line!"
                      << std::endl;
        }
    } 
}

// build module maps for speed access to module
// send the tdc group geometry to scene for annotation
void PRadEventViewer::buildModuleMap()
{
    handler->BuildChannelMap();
    // tdc maps
    std::unordered_map< std::string, PRadTDCGroup * > tdcList = handler->GetTDCGroupSet();
    for(auto &it : tdcList)
    {
        PRadTDCGroup *tdcGroup = it.second;
        std::vector< PRadDAQUnit* > groupList = tdcGroup->GetGroupList();

        if(!groupList.size())
            continue;

        // get id and set background color
        QString tdcGroupName = QString::fromStdString(tdcGroup->GetName());
        QColor bkgColor;
        int tdc = tdcGroupName.mid(1).toInt();
        if(tdcGroupName.at(0) == 'G') { // below is to make different color for adjacent groups
             if(tdc&1)
                bkgColor = QColor(255, 153, 153, 50);
             else
                bkgColor = QColor(204, 204, 255, 50);
        } else {
            if((tdc&1)^(((tdc-1)/6)&1))
                bkgColor = QColor(51, 204, 255, 50);
            else
                bkgColor = QColor(0, 255, 153, 50);
        }

        // get the tdc group box size
        double xmax = -600., xmin = 600.;
        double ymax = -600., ymin = 600.;
        bool has_module = false;
        for(auto &channel : groupList)
        {
            HyCalModule *module = dynamic_cast<HyCalModule *>(channel);
            if(module == nullptr)
                continue;

            has_module = true;
            HyCalModule::GeoInfo geo = module->GetGeometry();
            if((geo.x + geo.cellSize/2.) > xmax)
                xmax = geo.x + geo.cellSize/2.;
            if((geo.x - geo.cellSize/2.) < xmin)
                xmin = geo.x - geo.cellSize/2.;

            if((geo.y + geo.cellSize/2.) > ymax)
                ymax = geo.y + geo.cellSize/2.;
            if((geo.y - geo.cellSize/2.) < ymin)
                ymin = geo.y - geo.cellSize/2.;
        }
        QRectF groupBox = QRectF(xmin - HYCAL_SHIFT, ymin, xmax-xmin, ymax-ymin);
        if(has_module)
            HyCal->AddTextBox(tdcGroupName, groupBox, bkgColor);
    }
}

// read the pedestal data from previous file
void PRadEventViewer::readPedestalData(const QString &filename)
{
    QFile pedData(filename);

    if(!pedData.open(QFile::ReadOnly | QFile::Text)) {
        std::cout << "WARNING: Missing pedestal file \"" << filename.toStdString()
                  << "\", no pedestal data are read!" << std::endl;
        return;
    }

    int crate, slot, channel;
    double val, sigma;
    ChannelAddress daqInfo;
    PRadDAQUnit *tmp;

    QTextStream in(&pedData);

    while(!in.atEnd())
    {
        QString line = in.readLine().simplified();
        if(line.at(0) == '#')
            continue;
        QStringList fields = line.split(QRegExp("\\s+"));
        if(fields.size() == 5) {
            crate = fields.takeFirst().toInt();
            slot = fields.takeFirst().toInt();
            channel = fields.takeFirst().toInt();
            val = fields.takeFirst().toDouble();
            sigma = fields.takeFirst().toDouble();
            daqInfo.crate = crate;
            daqInfo.slot = slot;
            daqInfo.channel = channel;
            if((tmp = handler->FindChannel(daqInfo)) != nullptr)
                tmp->UpdatePedestal(val, sigma);
        }
    }

    pedData.close();
}

//============================================================================//
// HyCal Modules Actions                                                      //
//============================================================================//

// do the action for all modules
void PRadEventViewer::ModuleAction(void (HyCalModule::*act)())
{
    QVector<HyCalModule*> moduleList = HyCal->GetModuleList();
    for(auto &module : moduleList)
    {
        (module->*act)();
    }
}

void PRadEventViewer::ListModules()
{
    QVector<HyCalModule*> moduleList = HyCal->GetModuleList();
    std::ofstream outf("config/current_list.txt");

    for(auto &module : moduleList)
    {
        outf << std::setw(6)  << module->GetReadID().toStdString()
             << std::setw(10) << module->GetDAQInfo().crate
             << std::setw(6)  << module->GetDAQInfo().slot
             << std::setw(6)  << module->GetDAQInfo().channel
             << std::setw(6)  << module->GetTDCName()
             << std::setw(10) << (int)module->GetGeometry().type
             << std::setw(10) << module->GetGeometry().cellSize
             << std::setw(8)  << module->GetGeometry().x
             << std::setw(8)  << module->GetGeometry().y
//             << std::setw(10) << module->GetPedestal().mean
//             << std::setw(8)  << module->GetPedestal().sigma
             << std::setw(10) << module->GetHVInfo().crate
             << std::setw(6)  << module->GetHVInfo().slot
             << std::setw(6)  << module->GetHVInfo().channel
             << std::endl;
    }
}

//============================================================================//
// Get color, refresh and erase                                               //
//============================================================================//

// get color from spectrum
QColor PRadEventViewer::GetColor(const double &val)
{
    return energySpectrum->GetColor(val);
}

// refresh all the view
void PRadEventViewer::Refresh()
{
    switch(viewMode)
    {
    case PedestalView:
        ModuleAction(&HyCalModule::ShowPedestal);
        break;
    case SigmaView:
        ModuleAction(&HyCalModule::ShowPedSigma);
        break;
    case OccupancyView:
        ModuleAction(&HyCalModule::ShowOccupancy);
        break;
    case HighVoltageView:
        ModuleAction(&HyCalModule::ShowVoltage);
        break;
    case EnergyView:
        handler->UpdateEvent(event_index); // fetch data from handler
        ModuleAction(&HyCalModule::ShowEnergy);
        break;
    }

    UpdateStatusInfo();

    QWidget * viewport = view->viewport();
    viewport->update();
}

// clean all the data buffer
void PRadEventViewer::eraseModuleBuffer()
{
    handler->Clear();
    ModuleAction(&HyCalModule::CleanBuffer);
    updateEventRange();
}

//============================================================================//
// functions that react to menu, tool                                         //
//============================================================================//

// open file
void PRadEventViewer::openFile()
{
    QString codaData;
    codaData.sprintf("%s", getenv("CODA_DATA"));
    if (codaData.isEmpty())
        codaData = QDir::currentPath();

    QStringList filters;
    filters << "Data files (*.dat *.ev *.evio)"
            << "All files (*)";

    fileName = getFileName(tr("Choose a data file"), codaData, filters, "");

    if (!fileName.isEmpty()) {
        //TODO, dialog to notice waiting
//        QtConcurrent::run(this, &PRadEventViewer::readEventFromFile, fileName);
        readEventFromFile(fileName);
        UpdateStatusBar(DATA_FILE);
    }
}

// open pedestal file
void PRadEventViewer::openPedFile()
{
    QString codaData;
    codaData.sprintf("%s", getenv("CODA_DATA"));
    if (codaData.isEmpty())
        codaData = QDir::currentPath();

    QStringList filters;
    filters << "Data files (*.dat *.ev *.evio)"
            << "All files (*)";

    fileName = getFileName(tr("Choose a data file to generate pedestal"),
                           codaData,
                           filters,
                           "");

    if (!fileName.isEmpty()) {
        readEventFromFile(fileName);
        fitEventsForPedestal();
        UpdateStatusBar(NO_INPUT);
    }
}

void PRadEventViewer::changeHistType(int index)
{
    histType = (HistType)index;
    UpdateHistCanvas();
}

void PRadEventViewer::changeAnnoType(int index)
{
    annoType = (AnnoType)index;
    Refresh();
}

void PRadEventViewer::changeViewMode(int index)
{
    viewMode = (ViewMode)index;
    Refresh();
}

void PRadEventViewer::changeSpectrumSetting()
{
    if(specSetting->isVisible())
        specSetting->close();
    else
        specSetting->show();
}

void PRadEventViewer::eraseBufferAction()
{
    QMessageBox::StandardButton confirm;
    confirm = QMessageBox::question(this,
                                   "Erase Event Buffer",
                                   "Clear all the events, including histograms?",
                                    QMessageBox::Yes|QMessageBox::No);
    if(confirm == QMessageBox::Yes)
        eraseModuleBuffer();
}

void PRadEventViewer::UpdateStatusBar(ViewerStatus mode)
{
    QString statusText;
    switch(mode)
    {
    case NO_INPUT:
        statusText = tr("Please open a data file or use online mode.");
        break;
    case DATA_FILE:
        statusText = tr("Current Data File: ")+fileName;
        break;
    case ONLINE_MODE:
        statusText = tr("In online mode");
        break;
    }
    lStatusLabel->setText(statusText);
}

void PRadEventViewer::changeCurrentEvent(int evt)
{
    currentEvent = evt;
    Refresh();
}

void PRadEventViewer::updateEventRange()
{
    int total = handler->GetEventCount();

    eventSpin->setRange(1, total);
    if(total)
        eventCntLabel->setText(tr("Total events: ") + QString::number(total));
    else
        eventCntLabel->setText(tr("No events data loaded."));
    UpdateHistCanvas();
    Refresh();
}

void PRadEventViewer::UpdateHistCanvas()
{
    gSystem->ProcessEvents();
    switch(histType) {
    default:
    case ModuleHist:
        if(selection != nullptr) {
            PRadDAQUnit::Pedestal ped = selection->GetPedestal();
            int fit_min = int(ped.mean - 5*ped.sigma + 0.5);
            int fit_max = int(ped.mean + 5*ped.sigma + 0.5);
            histCanvas->UpdateHist(1, selection->GetHist(), fit_min, fit_max);
            PRadTDCGroup *tdc = handler->GetTDCGroup(selection->GetTDCName());
            if(tdc)
                histCanvas->UpdateHist(2, tdc->GetHist());
        }
        histCanvas->UpdateHist(3, handler->GetEnergyHist());
        break;
    case LMSHist:
        for(int i = 1; i <= 3; ++i)
        {
            PRadDAQUnit *lmsChannel = handler->FindChannel("LMS" + std::to_string(i));
            if(lmsChannel)
                histCanvas->UpdateHist(i, lmsChannel->GetHist());
        }
        break;
    }
}

void PRadEventViewer::SelectModule(HyCalModule* module)
{
    selection = module;
    UpdateHistCanvas();
    UpdateStatusInfo();
}

void PRadEventViewer::UpdateStatusInfo()
{
    if(selection == nullptr)
        return;

    QStringList valueList;
    QString typeInfo;

    ChannelAddress daqInfo = selection->GetDAQInfo();
    ChannelAddress hvInfo = selection->GetHVInfo();
    HyCalModule::GeoInfo geoInfo = selection->GetGeometry();

    if(geoInfo.type == HyCalModule::LeadTungstate) {
        typeInfo = tr("<center><p><b>PbWO<sub>4</sub></b></p></center>");
    } else {
        typeInfo = tr("<center><p><b>Pb-Glass</b></p></center>");
    }

    // first value column
    valueList << selection->GetReadID()                              // module ID
              << typeInfo                                            // module type
              << tr("C") + QString::number(daqInfo.crate)            // daq crate
                 + tr(", S") + QString::number(daqInfo.slot)         // daq slot
                 + tr(", Ch") + QString::number(daqInfo.channel)     // daq channel
              << QString::fromStdString(selection->GetTDCName())     // tdc group
              << tr("C") + QString::number(hvInfo.crate)             // hv crate
                 + tr(", S") + QString::number(hvInfo.slot)          // hv slot
                 + tr(", Ch") + QString::number(hvInfo.channel);     // hv channel

    PRadDAQUnit::Pedestal ped = selection->GetPedestal();
    HyCalModule::Voltage volt = selection->GetVoltage();
    QString temp = QString::number(volt.Vmon) + tr(" V ")
                   + ((volt.ON)? tr("/ ") : tr("(OFF) / "))
                   + QString::number(volt.Vset) + tr(" V");

    // second value column
    valueList << QString::number(ped.mean)                           // pedestal mean
#if QT_VERSION >= 0x050000
                 + tr(" \u00B1 ")
#else
                 + tr(" \261 ")
#endif
                 + QString::number(ped.sigma,'f',2)                  // pedestal sigma
              << QString::number(currentEvent)                       // current event
              << QString::number(selection->GetEnergy())+ tr(" MeV") // energy
              << QString::number(selection->GetOccupancy())          // occupancy
              << temp;                                               // HV info

    // update status info window
    for(int i = 0; i < 5; ++i)
    {
        statusItem[i]->setText(1, valueList.at(i));
        statusItem[i]->setText(3, valueList.at(5+i));
    }
}

void PRadEventViewer::readEventFromFile(const QString &filepath)
{
    std::cout << "Reading data from file " << filepath.toStdString() << std::endl;

    struct timeval timeStart, timeEnd;
    gettimeofday(&timeStart, nullptr);

    try {
        evio::evioFileChannel *chan = new evio::evioFileChannel(filepath.toStdString().c_str(),"r");
        chan->open();

        PRadEvioParser *myParser = new PRadEvioParser(handler);
        // Initialize

        eraseModuleBuffer();

        while(chan->read())
        {
            PRadEventHeader *evt = (PRadEventHeader*)chan->getBuffer();
            // check the event tag
            myParser->parseEventByHeader(evt);
         }
        delete myParser;
        chan->close();
        delete chan;
        updateEventRange();
    } catch (evio::evioException e) {
        std::cerr << e.toString() << endl;
    } catch (...) {
        std::cerr << "?unknown exception" << endl;
    }

    gettimeofday(&timeEnd, nullptr);

    std::cout << "Parsed " << handler->GetEventCount() << " events, took "
              << ((timeEnd.tv_sec - timeStart.tv_sec)*1000 + (timeEnd.tv_usec - timeStart.tv_usec)/1000)
              << " ms." << std::endl;
}

QString PRadEventViewer::getFileName(const QString &title,
                                     const QString &dir,
                                     const QStringList &filter,
                                     const QString &suffix,
                                     QFileDialog::AcceptMode mode)
{
    QString filepath;
    fileDialog->setWindowTitle(title);
    fileDialog->setDirectory(dir);
    fileDialog->setNameFilters(filter);
    fileDialog->setDefaultSuffix(suffix);
    fileDialog->setAcceptMode(mode);

    if(fileDialog->exec())
        filepath = fileDialog->selectedFiles().at(0);

    return filepath;
}

void PRadEventViewer::saveHistToFile()
{
    QString rootFile = getFileName(tr("Save histograms to root file"),
                                   tr("rootfiles/"),
                                   {tr("root files (*.root)")},
                                   tr("root"),
                                   QFileDialog::AcceptSave);

    if(rootFile.isEmpty()) // did not open a file
        return;

    TFile *f = new TFile(rootFile.toStdString().c_str(), "recreate");

    handler->GetEnergyHist()->Write();

    QVector<HyCalModule*> moduleList = HyCal->GetModuleList();
    for(auto &module : moduleList)
    {
        module->GetHist()->Write();
    }
    f->Close();
    rStatusLabel->setText(tr("All histograms are saved to ") + rootFile);
}

void PRadEventViewer::fitEventsForPedestal()
{
    logBox->TurnOffLog();

    ofstream pedestalmap("config/pedestal.dat");

    QVector<HyCalModule*> moduleList = HyCal->GetModuleList();
    for(auto &module : moduleList)
    {
        module->GetHist()->Fit("gaus");
        TF1 *myfit = (TF1*) module->GetHist()->GetFunction("gaus");
        double p0 = myfit->GetParameter(1);
        double p1 = myfit->GetParameter(2);
        module->UpdatePedestal(p0, p1);
        ChannelAddress daqInfo = module->GetDAQInfo();
        pedestalmap << daqInfo.crate << "  "
                    << daqInfo.slot << "  "
                    << daqInfo.channel << "  "
                    << p0 << "  "
                    << p1 << std::endl;
    }

    pedestalmap.close();

    logBox->TurnOnLog();
}

void PRadEventViewer::takeSnapShot()
{

#if QT_VERSION >= 0x050000
    QPixmap p = QGuiApplication::primaryScreen()->grabWindow(QApplication::activeWindow()->winId(), 0, 0);
#else
    QPixmap p = QPixmap::grabWindow(QApplication::activeWindow()->winId());
#endif

    // using date time as file name
    QString datetime = tr("snapshots/") + QDateTime::currentDateTime().toString();
    datetime.replace(QRegExp("\\s+"), "_");

    QString filepath = datetime + tr(".png");

    // make sure no snapshots are overwritten
    int i = 0;
    while(1) {
        QFileInfo check(filepath);
        if(!check.exists())
            break;
        ++i;
        filepath = datetime + tr("_") + QString::number(i) + tr(".png");
    }

    p.save(filepath);

    // update info
    rStatusLabel->setText(tr("Snap shot saved to ") + filepath);
}

void PRadEventViewer::handleRootEvents()
{
    gSystem->ProcessEvents();
}


//============================================================================//
// Online mode functions                                                      //
//============================================================================//
void PRadEventViewer::setupOnlineMode()
{
    etSetting = new ETSettingPanel(this);
    onlineTimer = new QTimer(this);
    connect(onlineTimer, SIGNAL(timeout()), this, SLOT(onlineUpdate()));

    // future watcher for online mode
    connect(&watcher, SIGNAL(finished()), this, SLOT(startOnlineMode()));

    etChannel = new PRadETChannel();
    hvSystem = new PRadHVSystem(handler);

    QFile hvCrateList("config/hv_crate_list.txt");

    if(!hvCrateList.open(QFile::ReadOnly | QFile::Text)) {
        std::cout << "WARNING: Missing HV crate list"
                  << "\" config/hv_crate_list.txt \", "
                  << "no HV crate added!"
                  << std::endl;
        return;
    }

    std::string name, ip;
    int id;

    QTextStream in(&hvCrateList);

    while(!in.atEnd())
    {
        QString line = in.readLine().simplified();
        if(line.at(0) == '#')
            continue;
        QStringList fields = line.split(QRegExp("\\s+"));
        if(fields.size() == 3) {
            name = fields.takeFirst().toStdString();
            ip = fields.takeFirst().toStdString();
            id = fields.takeFirst().toInt();
            hvSystem->AddCrate(name, ip, id);
        }
    }

    hvCrateList.close();
}

void PRadEventViewer::initOnlineMode()
{
    if(!etSetting->exec())
        return;

    // Disable buttons
    onlineEnAction->setEnabled(false);
    openDataAction->setEnabled(false);
    openPedAction->setEnabled(false);
    eventSpin->setEnabled(false);
    future = QtConcurrent::run(this, &PRadEventViewer::connectETClient);
    watcher.setFuture(future);
}

bool PRadEventViewer::connectETClient()
{
    try {
        etChannel->Open(etSetting->GetETHost().toStdString().c_str(),
                        etSetting->GetETPort(),
                        etSetting->GetETFilePath().toStdString().c_str());
        etChannel->NewStation(etSetting->GetStationName().toStdString());
        etChannel->AttachStation();
    } catch(PRadException e) {
        etChannel->ForceClose();
        std::cerr << e.FailureType() << ": "
                  << e.FailureDesc() << std::endl;
        return false;
    }

    return true;
}

void PRadEventViewer::startOnlineMode()
{
    if(!future.result()) { // did not connected to ET
        QMessageBox::critical(this,
                              "Online Mode",
                              "Failure in Open&Attach to ET!");

        rStatusLabel->setText(tr("Failed to start Online Mode!"));
        onlineEnAction->setEnabled(true);
        openDataAction->setEnabled(true);
        openPedAction->setEnabled(true);
        eventSpin->setEnabled(true);
        return;
    }

    QMessageBox::information(this,
                             tr("Online Mode"),
                             tr("Online Monitor Start!"));

    onlineDisAction->setEnabled(true);
    // Successfully attach to ET, change to online mode
    handler->OnlineMode();

    // Clean buffer
    eraseModuleBuffer();

    // Update to status bar
    UpdateStatusBar(ONLINE_MODE);

    // Start online timer
    onlineTimer->start(5000);
}

void PRadEventViewer::stopOnlineMode()
{
    // Stop timer
    onlineTimer->stop();

    etChannel->ForceClose();
    QMessageBox::information(this,
                             tr("Online Monitor"),
                             tr("Dettached from ET!"));

    handler->OfflineMode();

    // Enable buttons
    onlineEnAction->setEnabled(true);
    openDataAction->setEnabled(true);
    openPedAction->setEnabled(true);
    onlineDisAction->setEnabled(false);
    eventSpin->setEnabled(true);

    // Update to Main Window
    UpdateStatusBar(NO_INPUT);
}

void PRadEventViewer::onlineUpdate()
{
    try {
        int num;
        PRadEvioParser *myParser = new PRadEvioParser(handler);

        for(num = 0; etChannel->Read() && num < 10; ++num)
        {
            PRadEventHeader *evt = (PRadEventHeader*)etChannel->GetBuffer();
            myParser->parseEventByHeader(evt);
            // update current event information
            currentEvent = (int)myParser->GetCurrentEventNb();
        }

        if(num) {
            UpdateHistCanvas();
            Refresh();
        }

        delete myParser;
    } catch(PRadException e) {
        QMessageBox::critical(this,
                              QString::fromStdString(e.FailureType()),
                              QString::fromStdString(e.FailureDesc()));
        stopOnlineMode();
        return;
    }
}


//============================================================================//
// high voltage control functions                                             //
//============================================================================//

void PRadEventViewer::connectHVSystem()
{
    hvEnableAction->setEnabled(false);
    hvDisableAction->setEnabled(false);
    QtConcurrent::run(this, &PRadEventViewer::initHVSystem);
}

void PRadEventViewer::initHVSystem()
{
    hvSystem->Connect();
    hvDisableAction->setEnabled(true);
    hvMonitorAction->setEnabled(true);
}

void PRadEventViewer::startHVMonitor()
{
    hvSystem->StartMonitor();
    hvMonitorAction->setEnabled(false);
}

void PRadEventViewer::disconnectHVSystem()
{
    hvSystem->Disconnect();
    hvEnableAction->setEnabled(true);
    hvDisableAction->setEnabled(false);
    hvMonitorAction->setEnabled(false);
}
