//============================================================================//
// Main class for PRad Event Viewer, derived from QMainWindow                 //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include <QtGui>
#include <QTimer>

#include "TApplication.h"
#include "TSystem.h"
#include "TFile.h"
#include "TH1.h"
#include "TF1.h"

#include "evioUtil.hxx"
#include "evioFileChannel.hxx"

#include <stdio.h>
#include <fstream>
#include <iostream>

#include "PRadEventViewer.h"
#include "HyCalModule.h"
#include "HyCalScene.h"
#include "HyCalView.h"
#include "Spectrum.h"
#include "SpectrumSettingPanel.h"
#include "HtmlDelegate.h"
#include "PRadETChannel.h"
#include "ETSettingPanel.h"
#include "PRadEvioParser.h"
#include "PRadHistCanvas.h"
#include "PRadDataHandler.h"
#include "PRadHVChannel.h"
#include "PRadLogBox.h"

#include <sys/time.h>

#define cap_value(a, min, max) \
        (((a) >= (max)) ? (max) : ((a) <= (min)) ? (min) : (a))
#define event_index currentEvent - 1

//============================================================================//
// constructor                                                                //
//============================================================================//
PRadEventViewer::PRadEventViewer()
: currentEvent(0), selection(NULL), etChannel(NULL), hvChannel(NULL)
{
    initView();
    setupUI();
}

// set up the view for HyCal
void PRadEventViewer::initView()
{
    HyCal = new HyCalScene(this, -720, -720, 1440, 1440);
    HyCal->setBackgroundBrush(QColor(255, 255, 238));

    handler = new PRadDataHandler(); // data handler and container

    generateSpectrum();
    generateHyCalModules();

    setupOnlineMode();

    view = new HyCalView;
    view->setScene(HyCal);

    // root timer to process root events
    QTimer *rootTimer = new QTimer(this);
    connect(rootTimer, SIGNAL(timeout()), this, SLOT(handleRootEvents()));
    rootTimer->start(50);

    // future watcher for online mode
    connect(&watcher, SIGNAL(finished()), this, SLOT(onlineMode()));
}

// set up the UI
void PRadEventViewer::setupUI()
{
    setWindowTitle(tr("PRad Event Viewer"));
    QDesktopWidget dw;
    view->scale((dw.height()*0.8)/1440, (dw.height()*0.8)/1440);
    resize(dw.width()*0.8, dw.height()*0.8);

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
    readModuleList();
    readPedestalData("config/pedestal.dat");

    // Default setting
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

    connect(onlineEnAction, SIGNAL(triggered()), this, SLOT(startOnlineMode()));
    connect(onlineDisAction, SIGNAL(triggered()), this, SLOT(stopOnlineMode()));
    menuBar()->addMenu(onlineMenu);

    // tool menu, useful tools
    QMenu *toolMenu = new QMenu(tr("&Tools"));

    QAction *snapShotAction = toolMenu->addAction(tr("Take SnapShot"));
    snapShotAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_S));

    connect(snapShotAction, SIGNAL(triggered()), this, SLOT(takeSnapShot()));
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

    QPushButton *histCleanButton = new QPushButton("Erase Buffer");
    connect(histCleanButton, SIGNAL(clicked()),
            this, SLOT(eraseBufferAction()));

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

    connect(annoTypeBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeAnnoType(int)));
    connect(viewModeBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeViewMode(int)));
    connect(spectrumSettingButton, SIGNAL(clicked()),
            this, SLOT(changeSpectrumSetting()));

    PRadLogBox *logBox = new PRadLogBox();

    QGridLayout *layout = new QGridLayout();

    layout->addWidget(eventSpin,            0, 0, 1, 1);
    layout->addWidget(eventCntLabel,        0, 1, 1, 1);
    layout->addWidget(histCleanButton,      0, 2, 1, 1);
    layout->addWidget(viewModeBox,          1, 0, 1, 1);
    layout->addWidget(annoTypeBox,          1, 1, 1, 1);
    layout->addWidget(spectrumSettingButton,1, 2, 1, 1);
    layout->addWidget(logBox,               2, 0, 3, 3);

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
    QFont font("" , 9 , QFont::Bold );

    statusInfoTitle << tr("  Module Property  ") << tr("  Value  ")
                    << tr("  Module Property  ") << tr("  Value  ");
    statusInfoWidget->setHeaderLabels(statusInfoTitle);
    statusInfoWidget->setItemDelegate(new HtmlDelegate());
    statusInfoWidget->setIndentation(0);
    statusInfoWidget->setMaximumHeight(180);

    // add new items to status info
    QStringList statusProperty;
    statusProperty << tr("  Module ID") << tr("  Module Type") << tr("  DAQ Configuration") << tr("  TDC Group") << tr("  HV Configuration")
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
    statusItem[1]->useHtmlDelegate();

    statusInfoWidget->resizeColumnToContents(0);
    statusInfoWidget->resizeColumnToContents(2);

}

//============================================================================//
// read information from configuration files                                  //
//============================================================================//

// read module list from file
void PRadEventViewer::readModuleList()
{
    QFile list("config/module_list.txt");
    if(!list.open(QFile::ReadOnly | QFile::Text)) {
        std::cout << "ERROR: Missing configuration file \"config/module_list.txt\""
                  << ", cannot generate HyCal channels!"
                  << std::endl;
        exit(1);
    }

    QTextStream in(&list);
    QString moduleName;
    HyCalModule::DAQSetup daqInfo;
    HyCalModule::HVSetup hvInfo;
    HyCalModule::GeoInfo geometry;

    // some info that is not read from list
    // initialize first
    daqInfo.pedestal.mean = 0;
    daqInfo.pedestal.sigma = 0;
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
            daqInfo.config.crate = (unsigned char)fields.takeFirst().toInt();
            daqInfo.config.slot = (unsigned char)fields.takeFirst().toInt();
            daqInfo.config.channel = (unsigned char)fields.takeFirst().toInt();
            daqInfo.tdcGroup = (unsigned char)fields.takeFirst().toInt();
 
            geometry.type = (HyCalModule::ModuleType)fields.takeFirst().toInt();
            geometry.cellSize = fields.takeFirst().toDouble();
            geometry.x = fields.takeFirst().toDouble();
            geometry.y = fields.takeFirst().toDouble();

            hvInfo.config.crate = (unsigned char)fields.takeFirst().toInt();
            hvInfo.config.slot = (unsigned char)fields.takeFirst().toInt();
            hvInfo.config.channel = (unsigned char)fields.takeFirst().toInt();

            HyCalModule* newModule = new HyCalModule(this, moduleName, daqInfo, hvInfo, geometry);
            HyCal->addItem(newModule);
            handler->RegisterModule(newModule);
        } else {
            std::cout << "Unrecognized input format in configuration file, skipped one line!"
                      << std::endl;
        }
    }

    list.close();
    buildModuleMap();
}

// build module maps for speed access to module
// send the tdc group geometry to scene for annotation
void PRadEventViewer::buildModuleMap()
{
    handler->BuildModuleMap();

    // tdc maps
    std::vector< int > tdcList = handler->GetTDCGroupIDList();
    for(size_t i = 0; i < tdcList.size(); ++i)
    {
        std::vector< HyCalModule* > groupList = handler->GetTDCGroup(tdcList[i]);

        if(!groupList.size())
            continue;

        // start from a large enough number
        double xmax = -10000, ymax = -10000;
        double xmin = +10000, ymin = +10000;
        for(size_t j = 0; j < groupList.size(); ++j)
        {
            HyCalModule::GeoInfo geo = groupList[j]->GetGeometry();
            if((geo.x + geo.cellSize/2.) > xmax)
                xmax = geo.x + geo.cellSize/2.;
            if((geo.x - geo.cellSize/2.) < xmin)
                xmin = geo.x - geo.cellSize/2.;

            if((geo.y + geo.cellSize/2.) > ymax)
                ymax = geo.y + geo.cellSize/2.;
            if((geo.y - geo.cellSize/2.) < ymin)
                ymin = geo.y - geo.cellSize/2.;
        }
        QString tdcGroupName = groupList[0]->GetTDCGroupName();
        QRectF groupBox = QRectF(xmin - HYCAL_SHIFT, ymin, xmax-xmin, ymax-ymin);
        QColor bkgColor;
        if(tdcList[i] > 36) { // below is to make different color for adjacent groups
             if(tdcList[i]&1)
                bkgColor = QColor(255, 153, 153, 50);
             else
                bkgColor = QColor(204, 204, 255, 50);
        } else {
            if((tdcList[i]&1)^(((tdcList[i]-1)/6)&1))
                bkgColor = QColor(51, 204, 255, 50);
            else
                bkgColor = QColor(0, 255, 153, 50);
        }

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
    CrateConfig daqInfo;
    HyCalModule *tmp;

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
            if((tmp = handler->FindModule(daqInfo)) != NULL)
                tmp->UpdatePedestal(val, sigma);
        }
    }

    pedData.close();
}

//============================================================================//
// HyCal Modules Actions                                                      //
//============================================================================//

// do the action for all modules
void PRadEventViewer::ListBlockAction(void (HyCalModule::*act)())
{
    vector<HyCalModule*> moduleList = handler->GetModuleList();
    for(size_t i = 0; i < moduleList.size(); ++i) {
        (moduleList[i]->*act)();
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
        ListBlockAction(&HyCalModule::ShowPedestal);
        break;
    case SigmaView:
        ListBlockAction(&HyCalModule::ShowPedSigma);
        break;
    case OccupancyView:
        ListBlockAction(&HyCalModule::ShowOccupancy);
        break;
    case HighVoltageView:
        ListBlockAction(&HyCalModule::ShowVoltage);
        break;
    case EnergyView:
        handler->ShowEvent(event_index); // fetch data from handler
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
    ListBlockAction(&HyCalModule::CleanBuffer);
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

    fileName = getFileName(tr("Choose a data file"), codaData, "Data files (*.dat)", "");

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

    fileName = getFileName(tr("Choose a data file to generate pedestal"),
                           codaData,
                           "Data files (*.dat)",
                           "");

    if (!fileName.isEmpty()) {
        readEventFromFile(fileName);
        fitEventsForPedestal();
        eraseModuleBuffer();
        UpdateStatusBar(NO_INPUT);
    }
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
    if(selection != NULL) {
        HyCalModule::Pedestal ped = selection->GetPedestal();
        int fit_min = int(ped.mean - 5*ped.sigma + 0.5);
        int fit_max = int(ped.mean + 5*ped.sigma + 0.5);
        histCanvas->UpdateHist(1, selection->adcHist, fit_min, fit_max);
    }
    histCanvas->UpdateHist(2, handler->GetEnergyHist());
}

void PRadEventViewer::SelectModule(HyCalModule* module)
{
    selection = module;
    UpdateHistCanvas();
    UpdateStatusInfo();
}

void PRadEventViewer::UpdateStatusInfo()
{
    if(selection == NULL)
        return;

    QStringList valueList;
    QString typeInfo;

    CrateConfig daqInfo = selection->GetDAQInfo();
    CrateConfig hvInfo = selection->GetHVInfo();
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
                 + tr(", Ch") + QString::number(daqInfo.channel + 1) // daq channel
              << selection->GetTDCGroupName()                        // tdc group
              << tr("C") + QString::number(hvInfo.crate)             // hv crate
                 + tr(", S") + QString::number(hvInfo.slot)          // hv slot
                 + tr(", Ch") + QString::number(hvInfo.channel);     // hv channel

    HyCalModule::Pedestal ped = selection->GetPedestal();
    HyCalModule::Voltage volt = selection->GetVoltage();
    QString temp = (volt.ON)?(QString::number(volt.Vmon) + tr(" V / ")) : tr("OFF / ")
                   + QString::number(volt.Vset) + tr(" V");

    // second value column
    valueList << QString::number(ped.mean) + " \261 "                // pedestal mean
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
    gettimeofday(&timeStart, NULL);

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

    gettimeofday(&timeEnd, NULL);

    std::cout << "Parsed " << handler->GetEventCount() << " events, took "
              << ((timeEnd.tv_sec - timeStart.tv_sec)*1000 + (timeEnd.tv_usec - timeStart.tv_usec)/1000)
              << " ms." << std::endl;
}

QString PRadEventViewer::getFileName(const QString &title,
                                     const QString &dir,
                                     const QString &filter,
                                     const QString &suffix,
                                     QFileDialog::AcceptMode mode)
{
    QString filepath;
    fileDialog->setWindowTitle(title);
    fileDialog->setDirectory(dir);
    fileDialog->setNameFilter(filter);
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
                                   tr("root files (*.root)"),
                                   tr("root"),
                                   QFileDialog::AcceptSave);

    if(rootFile.isEmpty()) // did not open a file
        return;

    TFile *f = new TFile(rootFile.toStdString().c_str(), "recreate");

    handler->GetEnergyHist()->Write();

    vector<HyCalModule*> moduleList = handler->GetModuleList();
    for(size_t i = 0; i < moduleList.size(); ++i)
    {
        moduleList[i]->adcHist->Write();
    }
    f->Close();
    rStatusLabel->setText(tr("All histograms are saved to ") + rootFile);
}

void PRadEventViewer::fitEventsForPedestal()
{
    ofstream pedestalmap("config/pedestal.dat");

    vector<HyCalModule*> moduleList = handler->GetModuleList();
    for(size_t i = 0; i < moduleList.size(); ++i)
    {
        HyCalModule *tmp = moduleList[i];
        tmp->adcHist->Fit("gaus");
        TF1 *myfit = (TF1*) tmp->adcHist->GetFunction("gaus");
        double p0 = myfit->GetParameter(1);
        double p1 = myfit->GetParameter(2);
        tmp->UpdatePedestal(p0, p1);
        CrateConfig daqInfo = tmp->GetDAQInfo();
        pedestalmap << daqInfo.crate << "  "
                    << daqInfo.slot << "  "
                    << daqInfo.channel << "  "
                    << p0 << "  "
                    << p1 << std::endl;
    }

    pedestalmap.close();
}

void PRadEventViewer::takeSnapShot()
{
    QPixmap p = QPixmap::grabWindow(QApplication::activeWindow()->winId());

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

    hvChannel = new PRadHVChannel(handler);
    hvChannel->AddCrate("PRadHV_1", "129.57.160.67", 1);
    hvChannel->AddCrate("PRadHV_2", "129.57.160.68", 2);
    hvChannel->AddCrate("PRadHV_3", "129.57.160.69", 3);
    hvChannel->AddCrate("PRadHV_4", "129.57.160.70", 4);
    hvChannel->AddCrate("PRadHV_5", "129.57.160.71", 5);
}

void PRadEventViewer::startOnlineMode()
{
    if(!etSetting->exec())
        return;

    // Disable buttons
    onlineEnAction->setEnabled(false);
    openDataAction->setEnabled(false);
    openPedAction->setEnabled(false);
    eventSpin->setEnabled(false);
    QProgressBar bar(this);
    future = QtConcurrent::run(this, &PRadEventViewer::connectETClient);
    watcher.setFuture(future);
    bar.show();
}

bool PRadEventViewer::connectETClient()
{
    try {
        etChannel = new PRadETChannel(etSetting->GetETHost().toStdString().c_str(),
                                      etSetting->GetETPort(),
                                      etSetting->GetETFilePath().toStdString().c_str());
    } catch(PRadException e) {
        std::cerr << e.FailureType() << ": "
                  << e.FailureDesc() << std::endl;
        return false;
    }

    try {
        etChannel->CreateStation(etSetting->GetStationName().toStdString().c_str(), 2);
        etChannel->AttachStation();
    } catch(PRadException e) {
        delete etChannel;
        etChannel = NULL;
        std::cerr << e.FailureType() << ": "
                  << e.FailureDesc() << std::endl;
        return false;
    }
    std::cout << "Successfully attach to ET!" << std::endl;
    return true;
}

void PRadEventViewer::onlineMode()
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

    hvChannel->StartMonitor();

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

    hvChannel->StopMonitor();

    if(etChannel != NULL) {
        delete etChannel;
        etChannel = NULL;
        QMessageBox::information(this,
                                 tr("Online Monitor"),
                                 tr("Dettached from ET!"));
    }

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

