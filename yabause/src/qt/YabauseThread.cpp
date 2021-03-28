/*  Copyright 2005 Guillaume Duhamel
	Copyright 2005-2006, 2013 Theo Berkau
	Copyright 2008 Filipe Azevedo <pasnox@gmail.com>

	This file is part of Yabause.

	Yabause is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Yabause is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Yabause; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#include "YabauseThread.h"
#include "Settings.h"
#include "VolatileSettings.h"
#include "ui/UIPortManager.h"
#include "ui/UIYabause.h"

#include "../peripheral.h"

#include "vulkan/VIDVulkan.h"
#include "vulkan/VIDVulkanCInterface.h"

#include <QDateTime>
#include <QStringList>
#include <QDebug>

extern "C" {

  int YabauseThread_IsUseBios() {
    VolatileSettings* vs = QtYabause::volatileSettings();
    if (vs->value("General/EnableEmulatedBios", false).toBool()) {
      return -1;
    }
    else {
      return 0;
    }
    return -1;
  }

  const char * YabauseThread_getBackupPath() {
    yabauseinit_struct* conf = YabauseThread::getInstance()->yabauseConf();
    return conf->buppath;
  }

  void YabauseThread_setUseBios(int use) {

    VolatileSettings* vs = QtYabause::volatileSettings();
    if (use == 0) {
      vs->setValue("General/EnableEmulatedBios",true);
    }
    else {
      vs->setValue("General/EnableEmulatedBios", false);
    }

    yabauseinit_struct* conf = YabauseThread::getInstance()->yabauseConf();
    if (vs->value("General/EnableEmulatedBios", false).toBool())
      conf->biospath = strdup("");
    else
      conf->biospath = strdup(vs->value("General/Bios", conf->biospath).toString().toLatin1().constData());

  }

  void YabauseThread_setBackupPath( const char * buf) {
    VolatileSettings* vs = QtYabause::volatileSettings();
    vs->setValue("Memory/Path", buf);
    yabauseinit_struct* conf = YabauseThread::getInstance()->yabauseConf();
    conf->buppath = strdup(vs->value("Memory/Path", conf->buppath).toString().toLatin1().constData());
  }

  void YabauseThread_resetPlaymode() {
    VolatileSettings* vs = QtYabause::volatileSettings();
    vs->setValue("General/RecordDir", "");
    yabauseinit_struct* conf = YabauseThread::getInstance()->yabauseConf();
    conf->buppath = strdup(vs->value("General/RecordDir", conf->buppath).toString().toLatin1().constData());
  }

  void YabauseThread_coldBoot() {
    YabauseThread::getInstance()->pauseEmulation(false, true);
    YabauseThread::getInstance()->reset();
  }
}

YabauseThread * YabauseThread::instance = nullptr;

YabauseThread::YabauseThread( QObject* o )
	: QObject( o )
{
	mPause = true;
	mTimerId = -1;
	mInit = -1;
	memset(&mYabauseConf, 0, sizeof(mYabauseConf));
	showFPS = false;
  instance = this;
}

YabauseThread::~YabauseThread()
{
	deInitEmulation();
}

yabauseinit_struct* YabauseThread::yabauseConf()
{
	return &mYabauseConf;
}

void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
  VIDCore->Resize(0, 0, width, height, 0, 0);
}


bool YabauseThread::IsFullscreen(void)
{
  auto wnd = vulkanRenderer->getWindow()->getWindowHandle();
  return glfwGetWindowMonitor(wnd) != nullptr;
}


void YabauseThread::SetFullScreen(bool fullscreen)
{

  auto wnd = vulkanRenderer->getWindow()->getWindowHandle();
  
  if (IsFullscreen() == fullscreen)
    return;

  if (fullscreen)
  {
    // backup window position and window size
    glfwGetWindowPos(wnd, &_wndPos[0], &_wndPos[1]);
    glfwGetWindowSize(wnd, &_wndSize[0], &_wndSize[1]);

    // get resolution of monitor
    const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    // switch to full screen
    glfwSetWindowMonitor(wnd, _monitor, 0, 0, mode->width, mode->height, 0);
  }
  else
  {
    // restore last window size and position
    glfwSetWindowMonitor(wnd, nullptr, _wndPos[0], _wndPos[1], _wndSize[0], _wndSize[1], 0);
  }

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  YabauseThread * instance = YabauseThread::getInstance();

  if (action == GLFW_RELEASE) {

    if (key == GLFW_KEY_F4) {
      if (instance->IsFullscreen()) {
        instance->SetFullScreen(false);
      }
      else {
        instance->SetFullScreen(true);
      }
    }

  }
  

}

void YabauseThread::initEmulation()
{
	reloadSettings();

  VolatileSettings* vs = QtYabause::volatileSettings();
  int vidcoretype = vs->value("Video/VideoCore", mYabauseConf.vidcoretype).toInt();
  if (vidcoretype == VIDCORE_VULKAN) {
    int width = vs->value("Video/WinWidth", 800).toInt();
    int height = vs->value("Video/WinHeight", 600).toInt();

    vulkanRenderer = new Renderer();
    auto w = vulkanRenderer->OpenWindow(width, height, "[Yaba Sanshiro Vulkan] F4: Toggle full screen mode ", nullptr);
    VIDVulkan::getInstance()->setRenderer(vulkanRenderer);
    glfwSetFramebufferSizeCallback(w->getWindowHandle(), framebufferResizeCallback);
    glfwSetKeyCallback(w->getWindowHandle(), key_callback);
    _monitor = glfwGetPrimaryMonitor();
  }
	mInit = YabauseInit( &mYabauseConf );
	SetOSDToggle(showFPS);
}

void YabauseThread::deInitEmulation()
{
  VolatileSettings* vs = QtYabause::volatileSettings();
  int vidcoretype = vs->value("Video/VideoCore", mYabauseConf.vidcoretype).toInt();
  if (vidcoretype == VIDCORE_VULKAN) {
    vkQueueWaitIdle(vulkanRenderer->GetVulkanQueue());
    vkDeviceWaitIdle(vulkanRenderer->GetVulkanDevice());
  }
	YabauseDeInit();

  if (vidcoretype == VIDCORE_VULKAN) {
    vkQueueWaitIdle(vulkanRenderer->GetVulkanQueue());
    vkDeviceWaitIdle(vulkanRenderer->GetVulkanDevice());
    delete vulkanRenderer;
  }

	mInit = -1;
}

bool YabauseThread::pauseEmulation( bool pause, bool reset )
{
	if ( mPause == pause && !reset ) {
		return true;
	}
	
	if ( mInit == 0 && reset ) {
		deInitEmulation();
	}
	
	if ( mInit < 0 ) {
		initEmulation();
	}
	
	if ( mInit < 0 )
	{
		emit error( QtYabause::translate( "Can't initialize Yabause" ), false );
		return false;
	}
	
	mPause = pause;
	
	if ( mPause ) {
		ScspMuteAudio(SCSP_MUTE_SYSTEM);
		killTimer( mTimerId );
		mTimerId = -1;
	}
	else {
		ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
		mTimerId = startTimer( 0 );
	}
	
	VolatileSettings * vs = QtYabause::volatileSettings();

	if (vs->value("autostart").toBool())
	{
		if (vs->value("autostart/binary").toBool()) {
			MappedMemoryLoadExec(
				vs->value("autostart/binary/filename").toString().toLocal8Bit().constData(),
				vs->value("autostart/binary/address").toUInt());
		}
		else if (vs->value("autostart/load").toBool()) {
			YabLoadStateSlot( QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString().toLatin1().constData(), vs->value("autostart/load/slot").toInt() );
		}
		vs->setValue("autostart", false);
	}

	emit this->pause( mPause );
	
	return true;
}

bool YabauseThread::resetEmulation()
{
	if ( mInit < 0 ) {
		return false;
	}
	
	YabauseReset();
	
	emit reset();

	return true;
}

void YabauseThread::reloadControllers()
{
	PerPortReset();
	QtYabause::clearPadsBits();
	
	Settings* settings = QtYabause::settings();

	emit toggleEmulateMouse( false );

	for ( uint port = 1; port < 3; port++ )
	{
		settings->beginGroup( QString( "Input/Port/%1/Id" ).arg( port ) );
		QStringList ids = settings->childGroups();
		settings->endGroup();

    if (port == 1 && ids.size() == 0) {
      PerPad_struct* padbits = PerPadAdd(&PORTDATA1);
      PerSetKey(Qt::Key_Up, PERPAD_UP, padbits);
      PerSetKey(Qt::Key_Right, PERPAD_RIGHT, padbits);
      PerSetKey(Qt::Key_Down, PERPAD_DOWN, padbits);
      PerSetKey(Qt::Key_Left, PERPAD_LEFT, padbits);
      PerSetKey(Qt::Key_E, PERPAD_RIGHT_TRIGGER, padbits);
      PerSetKey(Qt::Key_Q, PERPAD_LEFT_TRIGGER, padbits);
      PerSetKey(Qt::Key_Return, PERPAD_START, padbits);
      PerSetKey(Qt::Key_Z, PERPAD_A, padbits);
      PerSetKey(Qt::Key_X, PERPAD_B, padbits);
      PerSetKey(Qt::Key_C, PERPAD_C, padbits);
      PerSetKey(Qt::Key_A, PERPAD_X, padbits);
      PerSetKey(Qt::Key_S, PERPAD_Y, padbits);
      PerSetKey(Qt::Key_D, PERPAD_Z, padbits);
      Settings* settings = QtYabause::settings();
      settings->setValue("Input/Port/1/Id/1/Type", 2);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/0", Qt::Key_Up);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/1", Qt::Key_Right);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/2", Qt::Key_Down);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/3", Qt::Key_Left);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/4", Qt::Key_E);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/5", Qt::Key_Q);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/6", Qt::Key_Return);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/7", Qt::Key_Z);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/8", Qt::Key_X);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/9", Qt::Key_C);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/10", Qt::Key_A);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/11", Qt::Key_S);
      settings->setValue("Input/Port/1/Id/1/Controller/2/Key/12", Qt::Key_D);
      return;
    }
		
		ids.sort();
		foreach ( const QString& id, ids )
		{
			uint type = settings->value( QString( UIPortManager::mSettingsType ).arg( port ).arg( id ) ).toUInt();
			
			switch ( type )
			{
				case PERPAD:
				{
					PerPad_struct* padbits = PerPadAdd( port == 1 ? &PORTDATA1 : &PORTDATA2 );
					
					settings->beginGroup( QString( "Input/Port/%1/Id/%2/Controller/%3/Key" ).arg( port ).arg( id ).arg( type ) );
					QStringList padKeys = settings->childKeys();
					settings->endGroup();
					
          padKeys.sort();
          foreach(const QString& padKey, padKeys) {
            const QString key = settings->value(QString(UIPortManager::mSettingsKey).arg(port).arg(id).arg(type).arg(padKey)).toString();
            PerSetKey(key.toUInt(), padKey.toUInt(), padbits);
          }
					break;
				}
				case PERWHEEL:
            {
               PerAnalog_struct* analogbits = PerWheelAdd(port == 1 ? &PORTDATA1 : &PORTDATA2);

               settings->beginGroup(QString("Input/Port/%1/Id/%2/Controller/%3/Key").arg(port).arg(id).arg(type));
               QStringList analogKeys = settings->childKeys();
               settings->endGroup();

               analogKeys.sort();
               foreach(const QString& analogKey, analogKeys)
               {
                  const QString key = settings->value(QString(UIPortManager::mSettingsKey).arg(port).arg(id).arg(type).arg(analogKey)).toString();

                  PerSetKey(key.toUInt(), analogKey.toUInt(), analogbits);
               }
               break;
            }
            case PERMISSIONSTICK:
            {
               PerAnalog_struct* analogbits = PerMissionStickAdd(port == 1 ? &PORTDATA1 : &PORTDATA2);

               settings->beginGroup(QString("Input/Port/%1/Id/%2/Controller/%3/Key").arg(port).arg(id).arg(type));
               QStringList analogKeys = settings->childKeys();
               settings->endGroup();

               analogKeys.sort();
               foreach(const QString& analogKey, analogKeys)
               {
                  const QString key = settings->value(QString(UIPortManager::mSettingsKey).arg(port).arg(id).arg(type).arg(analogKey)).toString();

                  PerSetKey(key.toUInt(), analogKey.toUInt(), analogbits);
               }
               break;
            }
            case PERTWINSTICKS:
            {
               PerAnalog_struct* analogbits = PerTwinSticksAdd(port == 1 ? &PORTDATA1 : &PORTDATA2);

               settings->beginGroup(QString("Input/Port/%1/Id/%2/Controller/%3/Key").arg(port).arg(id).arg(type));
               QStringList analogKeys = settings->childKeys();
               settings->endGroup();

               analogKeys.sort();
               foreach(const QString& analogKey, analogKeys)
               {
                  const QString key = settings->value(QString(UIPortManager::mSettingsKey).arg(port).arg(id).arg(type).arg(analogKey)).toString();

                  PerSetKey(key.toUInt(), analogKey.toUInt(), analogbits);
               }
               break;
            }
				case PER3DPAD:
				{
					PerAnalog_struct* analogbits = Per3DPadAdd( port == 1 ? &PORTDATA1 : &PORTDATA2 );

					settings->beginGroup( QString( "Input/Port/%1/Id/%2/Controller/%3/Key" ).arg( port ).arg( id ).arg( type ) );
					QStringList analogKeys = settings->childKeys();
					settings->endGroup();

					analogKeys.sort();
					foreach ( const QString& analogKey, analogKeys )
					{
						const QString key = settings->value( QString( UIPortManager::mSettingsKey ).arg( port ).arg( id ).arg( type ).arg( analogKey ) ).toString();

						PerSetKey( key.toUInt(), analogKey.toUInt(), analogbits );
					}
					break;
				}
				case PERGUN:
				{
					PerGun_struct* gunbits = PerGunAdd( port == 1 ? &PORTDATA1 : &PORTDATA2 );
					settings->beginGroup( QString( "Input/Port/%1/Id/%2/Controller/%3/Key" ).arg( port ).arg( id ).arg( type ) );
					QStringList gunKeys = settings->childKeys();
					settings->endGroup();

					gunKeys.sort();
					foreach ( const QString& gunKey, gunKeys )
					{
						const QString key = settings->value( QString( UIPortManager::mSettingsKey ).arg( port ).arg( id ).arg( type ).arg( gunKey ) ).toString();

						PerSetKey( key.toUInt(), gunKey.toUInt(), gunbits );
					}
					break;
				}
				case PERKEYBOARD:
					QtYabause::mainWindow()->appendLog( "Keyboard controller type is not yet supported" );
					break;
				case PERMOUSE:
				{
					PerMouse_struct* mousebits = PerMouseAdd( port == 1 ? &PORTDATA1 : &PORTDATA2 );

					settings->beginGroup( QString( "Input/Port/%1/Id/%2/Controller/%3/Key" ).arg( port ).arg( id ).arg( type ) );
					QStringList mouseKeys = settings->childKeys();
					settings->endGroup();

					mouseKeys.sort();
					foreach ( const QString& mouseKey, mouseKeys )
					{
						const QString key = settings->value( QString( UIPortManager::mSettingsKey ).arg( port ).arg( id ).arg( type ).arg( mouseKey ) ).toString();

						PerSetKey( key.toUInt(), mouseKey.toUInt(), mousebits );
					}

					emit toggleEmulateMouse( true );
					break;
				}
				case 0:
					// Unconnected
					break;
				default:
					QtYabause::mainWindow()->appendLog( "Invalid controller type" );
					break;
			}
		}
	}
}

static struct tm *localtime_qt(const QDateTime &input, struct tm *result)
{
	QDate date(input.date());
	result->tm_year = date.year() - 1900;
	result->tm_mon = date.month();
	result->tm_mday = date.day();
	result->tm_wday = date.dayOfWeek();
	result->tm_yday = date.dayOfYear();

	QTime time(input.time());
	result->tm_sec = time.second();
	result->tm_min = time.minute();
	result->tm_hour = time.hour();

	return result;
}

void YabauseThread::reloadClock()
{
	QString tmp;
	Settings* s = QtYabause::settings();

	if (mYabauseConf.basetime == 0)
		tmp = "";
	else
		tmp = QDateTime::fromTime_t(mYabauseConf.basetime).toString();

	// Clock sync
	mYabauseConf.clocksync = (int)s->value( "General/ClockSync", mYabauseConf.clocksync ).toBool();
	tmp = s->value( "General/FixedBaseTime", tmp ).toString();
	if (!tmp.isEmpty() && mYabauseConf.clocksync) 
	{
		QDateTime date = QDateTime::fromString(tmp, Qt::ISODate);
		mYabauseConf.basetime = (long)date.toTime_t();	
	}
	else {
		mYabauseConf.basetime = 0;
	}
}

void YabauseThread::reloadSettings()
{
	//QMutexLocker l( &mMutex );
	// get settings pointer
	VolatileSettings* vs = QtYabause::volatileSettings();
	
	// reset yabause conf
	resetYabauseConf();

	// read & apply settings
   mYabauseConf.m68kcoretype = vs->value("Advanced/68kCore", mYabauseConf.m68kcoretype).toInt();
	mYabauseConf.percoretype = vs->value( "Input/PerCore", mYabauseConf.percoretype ).toInt();
	mYabauseConf.sh2coretype = vs->value( "Advanced/SH2Interpreter", mYabauseConf.sh2coretype ).toInt();
	mYabauseConf.vidcoretype = vs->value( "Video/VideoCore", mYabauseConf.vidcoretype ).toInt();
	mYabauseConf.osdcoretype = vs->value( "Video/OSDCore", mYabauseConf.osdcoretype ).toInt();
	mYabauseConf.sndcoretype = vs->value( "Sound/SoundCore", mYabauseConf.sndcoretype ).toInt();
	mYabauseConf.cdcoretype = vs->value( "General/CdRom", mYabauseConf.cdcoretype ).toInt();
	mYabauseConf.carttype = vs->value( "Cartridge/Type", mYabauseConf.carttype ).toInt();
	const QString r = vs->value( "Advanced/Region", mYabauseConf.regionid ).toString();
	if ( r.isEmpty() || r == "Auto" )
		mYabauseConf.regionid = 0;
	else
	{
		switch ( r[0].toLatin1() )
		{
			case 'J': mYabauseConf.regionid = 1; break;
			case 'T': mYabauseConf.regionid = 2; break;
			case 'U': mYabauseConf.regionid = 4; break;
			case 'B': mYabauseConf.regionid = 5; break;
			case 'K': mYabauseConf.regionid = 6; break;
			case 'A': mYabauseConf.regionid = 0xA; break;
			case 'E': mYabauseConf.regionid = 0xC; break;
			case 'L': mYabauseConf.regionid = 0xD; break;
		}
	}
	if (vs->value("General/EnableEmulatedBios", false).toBool())
		mYabauseConf.biospath = strdup( "" );
	else
		mYabauseConf.biospath = strdup( vs->value( "General/Bios", mYabauseConf.biospath ).toString().toLatin1().constData() );

  mYabauseConf.framelimit = vs->value("General/EmulationSpeed", mYabauseConf.framelimit).toInt();
	mYabauseConf.cdpath = strdup( vs->value( "General/CdRomISO", mYabauseConf.cdpath ).toString().toLatin1().constData() );
   mYabauseConf.ssfpath = strdup(vs->value("General/SSFPath", mYabauseConf.ssfpath).toString().toLatin1().constData());
   mYabauseConf.play_ssf = vs->value("General/PlaySSF", false).toBool();
   showFPS = vs->value( "General/ShowFPS", false ).toBool();
	mYabauseConf.usethreads = (int)vs->value( "General/EnableMultiThreading", mYabauseConf.usethreads ).toBool();
	mYabauseConf.numthreads = vs->value( "General/NumThreads", mYabauseConf.numthreads ).toInt();
	mYabauseConf.buppath = strdup( vs->value( "Memory/Path", mYabauseConf.buppath ).toString().toLatin1().constData() );
	mYabauseConf.mpegpath = strdup( vs->value( "MpegROM/Path", mYabauseConf.mpegpath ).toString().toLatin1().constData() );
  if (vs->value("Memory/ExtendMemory", true).toBool()) {
    mYabauseConf.extend_backup = 1;
  }else {
    mYabauseConf.extend_backup = 0;
  }
	mYabauseConf.cartpath = strdup( vs->value( "Cartridge/Path", mYabauseConf.cartpath ).toString().toLatin1().constData() );
	mYabauseConf.modemip = strdup( vs->value( "Cartridge/ModemIP", mYabauseConf.modemip ).toString().toLatin1().constData() );
	mYabauseConf.modemport = strdup( vs->value( "Cartridge/ModemPort", mYabauseConf.modemport ).toString().toLatin1().constData() );
	mYabauseConf.videoformattype = vs->value( "Video/VideoFormat", mYabauseConf.videoformattype ).toInt();
   mYabauseConf.use_new_scsp = (int)vs->value("Sound/NewScsp", mYabauseConf.use_new_scsp).toBool();

	mYabauseConf.video_filter_type = vs->value("Video/filter_type", mYabauseConf.video_filter_type).toInt();
	mYabauseConf.polygon_generation_mode = vs->value("Video/polygon_generation_mode", mYabauseConf.polygon_generation_mode).toInt();
  mYabauseConf.resolution_mode = vs->value("Video/resolution_mode", mYabauseConf.resolution_mode).toInt();
  mYabauseConf.rbg_resolution_mode = vs->value("Video/rbg_resolution_mode", mYabauseConf.rbg_resolution_mode).toInt();
  mYabauseConf.rbg_use_compute_shader = vs->value("Video/UseComputeShader", mYabauseConf.rbg_use_compute_shader).toBool();

	emit requestSize( QSize( vs->value( "Video/WinWidth", 0 ).toInt(), vs->value( "Video/WinHeight", 0 ).toInt() ) );
	emit requestFullscreen( vs->value( "Video/Fullscreen", false ).toBool() );
	emit requestVolumeChange( vs->value( "Sound/Volume", 100 ).toInt() );


  mYabauseConf.rotate_screen = vs->value("Video/RotateScreen", false).toBool() ;
  mYabauseConf.scsp_sync_count_per_frame = vs->value("Sound/ScspSync", 1).toInt();
  mYabauseConf.scsp_main_mode = vs->value("Sound/ScspMainMode", 1).toInt();

  mYabauseConf.playRecordPath = strdup(vs->value("General/RecordDir", mYabauseConf.playRecordPath).toString().toLatin1().constData());

	reloadClock();
	reloadControllers();
}

bool YabauseThread::emulationRunning()
{
	//QMutexLocker l( &mMutex );
	return mTimerId != -1 && !mPause;
}

bool YabauseThread::emulationPaused()
{
	//QMutexLocker l( &mMutex );
	return mPause;
}

void YabauseThread::OpenTray(){
	Cs2ForceOpenTray();
}

void YabauseThread::CloseTray(){

	VolatileSettings* vs = QtYabause::volatileSettings();
	mYabauseConf.cdcoretype = vs->value("General/CdRom", mYabauseConf.cdcoretype).toInt();
	mYabauseConf.cdpath = strdup(vs->value("General/CdRomISO", mYabauseConf.cdpath).toString().toLatin1().constData());

	Cs2ForceCloseTray(mYabauseConf.cdcoretype, mYabauseConf.cdpath);
}

void YabauseThread::resetYabauseConf()
{
	// free structure
	memset( &mYabauseConf, 0, sizeof( yabauseinit_struct ) );
	// fill default structure
  mYabauseConf.m68kcoretype = QtYabause::default68kCore().id;
	mYabauseConf.percoretype = QtYabause::defaultPERCore().id;
  mYabauseConf.sh2coretype = QtYabause::defaultCpuCore()->id;
	mYabauseConf.vidcoretype = QtYabause::defaultVIDCore().id;
	mYabauseConf.sndcoretype = QtYabause::defaultSNDCore().id;
	mYabauseConf.cdcoretype = QtYabause::defaultCDCore().id;
	mYabauseConf.carttype = CART_NONE;
	mYabauseConf.regionid = 0;
	mYabauseConf.biospath = 0;
	mYabauseConf.cdpath = 0;
	mYabauseConf.mpegpath = 0;
	mYabauseConf.cartpath = 0;
	mYabauseConf.videoformattype = VIDEOFORMATTYPE_NTSC;
	mYabauseConf.skip_load = 0;
	int numThreads = QThread::idealThreadCount();	
	mYabauseConf.usethreads = numThreads <= 1 ? 0 : 1;
	mYabauseConf.numthreads = numThreads < 0 ? 1 : numThreads;
	mYabauseConf.video_filter_type = 0;
	mYabauseConf.polygon_generation_mode = 0;
  mYabauseConf.resolution_mode = 0;
  mYabauseConf.framelimit = 0;
  mYabauseConf.rbg_resolution_mode = 0;
  mYabauseConf.rbg_use_compute_shader = 0;
  mYabauseConf.rotate_screen = 0;
  mYabauseConf.scsp_sync_count_per_frame = 1;
  mYabauseConf.scsp_main_mode = 1;
  mYabauseConf.use_new_scsp = 1;
  mYabauseConf.buppath = strdup(getDataDirPath().append("/bkram.bin").toLatin1().constData());
  mYabauseConf.playRecordPath = NULL;
}

void YabauseThread::timerEvent( QTimerEvent* )
{
	//mPause = true;
	//mRunning = true;
	//while ( mRunning )
	{
		if ( !mPause )
			PERCore->HandleEvents();
		//else
			//msleep( 25 );
		//sleep( 0 );
	}
}
