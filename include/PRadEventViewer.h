#ifndef PRAD_EVENT_VIEWER_H
#define PRAD_EVENT_VIEWER_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFutureWatcher>
#include <vector>

class HyCalScene;
class HyCalView;
class HyCalModule;
class Spectrum;
class SpectrumSettingPanel;
class PRadHistCanvas;
class PRadDataHandler;
class PRadLogBox;
class PRadHyCalCluster;
class PRadGEMSystem;
class PRadDetCoor;
class PRadDetMatch;
#ifdef USE_ONLINE_MODE
class PRadETChannel;
class ETSettingPanel;
#endif
#ifdef USE_CAEN_HV
class PRadHVSystem;
#endif

//class GEM;
class TCanvas;
class TObject;
class TH1D;

QT_BEGIN_NAMESPACE
class QPushButton;
class QComboBox;
class QSpinBox;
class QSlider;
class QString;
class QLabel;
class QSplitter;
class QTreeWidget;
class QTreeWidgetItem;
class QTimer;
class QAction;
QT_END_NAMESPACE

enum HistType {
    EnergyTDCHist,
    ModuleHist,
    TaggerHist,
};

enum AnnoType {
    NoAnnotation,
    ShowID,
    ShowDAQ,
    ShowTDC,
};

enum ViewMode {
    EnergyView,
    OccupancyView,
    PedestalView,
    SigmaView,
#ifdef USE_CAEN_HV
    HighVoltageView,
    VoltageSetView,
#endif
    CustomView,
};

enum ViewerStatus {
    NO_INPUT,
    DATA_FILE,
    ONLINE_MODE,
};

class PRadEventViewer : public QMainWindow
{
    Q_OBJECT

public:
    PRadEventViewer();
    virtual ~PRadEventViewer();
    template<typename... Args>
    void ModuleAction(void (HyCalModule::*act)(Args...), Args&&... args);
    void ListModules();
    ViewMode GetViewMode() {return viewMode;};
    AnnoType GetAnnoType() {return annoType;};
    QColor GetColor(const double &val);
    void UpdateStatusBar(ViewerStatus mode);
    void UpdateStatusInfo();
    void UpdateHistCanvas();
    void SelectModule(HyCalModule* module);
    PRadDataHandler *GetHandler() {return handler;};

public slots:
    void Refresh();

private slots:
    void openDataFile();
    void initializeFromFile();
    void openPedFile();
    void openCalibrationFile();
    void openGainFactorFile();
    void openCustomMap();
    void handleRootEvents();
    void saveHistToFile();
    void savePedestalFile();
    void findPeak();
    void fitPedestal();
    void fitHistogram();
    void correctGainFactor();
    void takeSnapShot();
    void changeHistType(int index);
    void changeAnnoType(int index);
    void changeViewMode(int index);
    void changeSpectrumSetting();
    void changeCurrentEvent(int evt);
    void eraseBufferAction();
    void findEvent();
    void editCustomValueLabel(QTreeWidgetItem* item, int column);
    void useSquareRecon();
    void useIslandRecon();
    void showAllGEMHits() { fShowMatchedGEM = false; }
    void showMatchedGEMHits() { fShowMatchedGEM = true; }

private:
    void initView();
    void setupUI();
    void generateSpectrum();
    void generateHyCalModules();
    void generateScalerBoxes();
    void setTDCGroupBox();
    void readModuleList();
    void readTDCList();
    void readSpecialChannels();
    void eraseModuleBuffer();
    void createMainMenu();
    void createControlPanel();
    void createStatusBar();
    void createStatusWindow();
    void setupInfoWindow();
    void updateEventRange();
    void readEventFromFile(const QString &filepath);
    void readCustomValue(const QString &filepath);
    bool onlineSettings();
    void onlineUpdate(const size_t &max_events);
    QString getFileName(const QString &title,
                        const QString &dir,
                        const QStringList &filter,
                        const QString &suffix,
                        QFileDialog::AcceptMode mode = QFileDialog::AcceptOpen);
    QStringList getFileNames(const QString &title,
                             const QString &dir,
                             const QStringList &filter,
                             const QString &suffix,
                             QFileDialog::AcceptMode mode = QFileDialog::AcceptOpen,
                             QFileDialog::FileMode fmode = QFileDialog::ExistingFiles);
    void reconCurrentEvent();

    PRadDataHandler *handler;
    PRadDetCoor *fDetCoor;
    PRadDetMatch *fDetMatch;
    int currentEvent;
    HistType histType;
    AnnoType annoType;
    ViewMode viewMode;

    HyCalModule *selection;
    Spectrum *energySpectrum;
    //GEM *myGEM;
    HyCalScene *HyCal;
    HyCalView *view;
    PRadHistCanvas *histCanvas;

    QString fileName;

    QSplitter *statusWindow;
    QSplitter *rightPanel;
    QSplitter *mainSplitter;

    QWidget *controlPanel;
    QSpinBox *eventSpin;
    QLabel *eventCntLabel;
    QComboBox *histTypeBox;
    QComboBox *annoTypeBox;
    QComboBox *viewModeBox;
    QPushButton *spectrumSettingButton;

    QTreeWidget *statusInfoWidget;
    QTreeWidgetItem *statusItem[6];

    QLabel *lStatusLabel;
    QLabel *rStatusLabel;

    QAction *openDataAction;

    QFileDialog *fileDialog;
    SpectrumSettingPanel *specSetting;
    PRadLogBox *logBox;

    QFuture<bool> future;
    QFutureWatcher<void> watcher;

    bool fUseIsland;
    bool fShowMatchedGEM;

#ifdef USE_ONLINE_MODE
public:
    void UpdateOnlineInfo();
private slots:
    void initOnlineMode();
    bool connectETClient();
    void startOnlineMode();
    void stopOnlineMode();
    void handleOnlineTimer();
private:
    void setupOnlineMode();
    QMenu *setupOnlineMenu();
    PRadETChannel *etChannel;
    QTimer *onlineTimer;
    ETSettingPanel *etSetting;
    QAction *onlineEnAction;
    QAction *onlineDisAction;
#endif

#ifdef USE_CAEN_HV
signals:
    void HVSystemInitialized();
private slots:
    void connectHVSystem();
    void initHVSystem();
    void disconnectHVSystem();
    void startHVMonitor();
    void saveHVSetting();
    void restoreHVSetting();
private:
    PRadHVSystem *hvSystem;
    QMenu *setupHVMenu();
    void setupHVSystem();
    QAction *hvEnableAction;
    QAction *hvDisableAction;
    QAction *hvSaveAction;
    QAction *hvRestoreAction;
#endif
};

#endif
