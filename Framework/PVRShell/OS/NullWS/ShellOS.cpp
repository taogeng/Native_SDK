/*!*********************************************************************************************************************
\file         PVRShell\OS\NullWS\ShellOS.cpp
\author       PowerVR by Imagination, Developer Technology Team
\copyright    Copyright (c) Imagination Technologies Limited.
\brief     	  Contains the implementation for the pvr::system::ShellOS class on Null Windowing System platforms (Linux & Neutrino).
***********************************************************************************************************************/
//!\cond NO_DOXYGEN
#include "PVRShell/OS/ShellOS.h"
#include "PVRCore/FilePath.h"
#include "PVRCore/Log.h"

#if defined(__linux__)

#include <sys/stat.h>
#include <fcntl.h>
#include <csignal>
#include <termios.h>
#include <unistd.h>
#include <cstdarg>
#include <cstdio>
#include <cerrno>

#include <linux/input.h>

#if !defined(CONNAME)
#define CONNAME "/dev/tty"
#endif

#if !defined(KEYPAD_INPUT)
#define KEYPAD_INPUT "/dev/input/event1"
#endif

#else
#include <windows.h>
#endif
using namespace pvr::types;
namespace pvr {
namespace system {
struct InternalOS
{
	bool isInitialized;
	uint32 display;

#if defined(__linux__)
	int devfd;
	struct termios termio;
	struct termios termio_orig;
	int keypad_fd;
	int keyboard_fd;
	bool keyboardShiftHeld; // Is one of the shift keys on the keyboard being held down? (Keyboard device only - not terminal).
#endif

	InternalOS() : isInitialized(false), display(0)
#if defined(__linux__)
		, devfd(0), keypad_fd(0), keyboard_fd(0), keyboardShiftHeld(false)
#endif
	{

	}

	~InternalOS()
	{
#if defined(__linux__)
		// Recover tty state.
		tcsetattr(devfd, TCSANOW, &termio_orig);
#endif
	}

#if defined(__linux__)
	Keys::Enum getSpecialKey(Keys::Enum firstCharacter) const;
#endif

};

// Setup the capabilities.
const ShellOS::Capabilities ShellOS::m_capabilities = { Capability::Unsupported, Capability::Unsupported };

ShellOS::ShellOS(OSApplication application, OSDATA osdata) : m_instance(application)
{
	m_OSImplementation = new InternalOS;
}

ShellOS::~ShellOS()
{
	delete m_OSImplementation;
}

void ShellOS::updatePointingDeviceLocation()
{
	static bool runOnlyOnce = true;
	if (runOnlyOnce)
	{
		m_shell->updatePointerPosition(PointerLocation(0, 0));
		runOnlyOnce = false;
	}
}

Result::Enum ShellOS::init(DisplayAttributes& data)
{
	if (!m_OSImplementation)
	{
		return Result::OutOfMemory;
	}

#if defined(__linux__)
	// In case we're in the background ignore SIGTTIN and SIGTTOU.
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	// Keyboard handling.
	if ((m_OSImplementation->devfd = open(CONNAME, O_RDWR | O_NDELAY)) <= 0)
	{
		Log(Log.Warning, "Can't open tty '" CONNAME "'");
	}
	else
	{
		tcgetattr(m_OSImplementation->devfd, &m_OSImplementation->termio_orig);
		tcgetattr(m_OSImplementation->devfd, &m_OSImplementation->termio);
		cfmakeraw(&m_OSImplementation->termio);
		m_OSImplementation->termio.c_oflag |= OPOST | ONLCR; // Turn back on cr-lf expansion on output
		m_OSImplementation->termio.c_cc[VMIN] = 1;
		m_OSImplementation->termio.c_cc[VTIME] = 0;

		if (tcsetattr(m_OSImplementation->devfd, TCSANOW, &m_OSImplementation->termio) == -1)
		{
			Log(Log.Warning, "Can't set tty attributes for '" CONNAME "'");
		}
	}

	// Keypad handling.
	if ((m_OSImplementation->keypad_fd = open(KEYPAD_INPUT, O_RDONLY | O_NDELAY)) <= 0)
	{
		Log(Log.Warning, "Can't open keypad input device (%s)\n", KEYPAD_INPUT);
	}

	// Keyboard handling.
	// Get keyboard device file name.
	// Uses the fact that all device information is shown in /proc/bus/input/devices and
	// the keyboard device file should always have an EV of 120013.
	static const char* command =
	  "grep -E 'Handlers|EV' /proc/bus/input/devices |"
	  "grep -B1 120013 |"
	  "grep -Eo event[0-9]+ |"
	  "tr '\\n' '\\0'";

	FILE* pipe = popen(command, "r");
	if (pipe == NULL)
	{
		Log(Log.Warning, "Can't find keyboard input device\n");
	}

	char devFilePath[20] = "/dev/input/";
	char temp[9];
	memset(temp, 0, sizeof(temp));
	if (fgets(temp, 9, pipe));
	pclose(pipe);

	static char* keyboardDeviceFileName = strdup(strcat(devFilePath, temp));

	m_OSImplementation->keyboard_fd = open(keyboardDeviceFileName, O_RDONLY | O_NDELAY);
	if (m_OSImplementation->keyboard_fd <= 0)
	{
		Log(Log.Warning, "Can't open keyboard input device (%s)  -- (Code : %i - %s)\n", keyboardDeviceFileName, errno, strerror(errno));
	}

	// Construct our read and write path.
	pid_t ourPid = getpid();
	char* exePath, srcLink[64];
	int len = 512;
	int res;

	sprintf(srcLink, "/proc/%d/exe", ourPid);
	exePath = 0;

	do
	{
		len *= 2;
		delete[] exePath;
		exePath = new char[len];
		res = readlink(srcLink, exePath, len);

		if (res < 0)
		{
			Log(Log.Warning, "Readlink %s failed. The application name, read path and write path have not been set.\n", exePath);
			break;
		}
	}
	while (res >= len);

	if (res >= 0)
	{
		exePath[res] = '\0'; // Null-terminate readlink's result.
		FilePath filepath(exePath);
		setApplicationName(filepath.getFilenameNoExtension());
		m_WritePath = filepath.getDirectory() + FilePath::getDirectorySeparator();
		m_ReadPaths.clear();
		m_ReadPaths.push_back(filepath.getDirectory() + FilePath::getDirectorySeparator());
		m_ReadPaths.push_back(std::string(".") + FilePath::getDirectorySeparator());
		m_ReadPaths.push_back(filepath.getDirectory() + FilePath::getDirectorySeparator() + "Assets" + FilePath::getDirectorySeparator());
	}

	delete[] exePath;

	/*
	 Get rid of the blinking cursor on a screen.

	 It's an equivalent of:
	 echo -n -e "\033[?25l" > /dev/tty0

	 if you do the above command then you can undo it with:
	 echo -n -e "\033[?25h" > /dev/tty0
	 */
	FILE* tty = 0;
	tty = fopen("/dev/tty0", "w");
	if (tty != 0)
	{
		const char txt[] = { 27 /* the ESCAPE ASCII character */
		                     , '['
		                     , '?'
		                     , '2'
		                     , '5'
		                     , 'l'
		                     , 0
		                   };

		fprintf(tty, "%s", txt);
		fclose(tty);
	}
#endif
	return Result::Success;
}

Result::Enum ShellOS::initializeWindow(DisplayAttributes& data)
{
	m_OSImplementation->isInitialized = true;
	data.fullscreen = true;
	data.x = data.y = 0;
    data.width = data.height = 0; //no way of getting the monitor resolution.
	return Result::Success;
}

void ShellOS::releaseWindow()
{
	m_OSImplementation->isInitialized = false;

	close(m_OSImplementation->keyboard_fd);
	close(m_OSImplementation->keypad_fd);
}

OSApplication ShellOS::getApplication() const
{
	return m_instance;
}

OSDisplay ShellOS::getDisplay() const
{
	return reinterpret_cast<OSDisplay>(m_OSImplementation->display);
}

OSWindow ShellOS::getWindow() const
{
	return NULL;
}

static Keys::Enum terminalStandardKeyMap[128] =
{
	Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown,                /* 0   */
	Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Backspace, Keys::Tab,                  /* 5   */
	Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Return, Keys::Unknown,                 /* 10  */
	Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown,                /* 15  */
	Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown,                /* 20  */
	Keys::Unknown, Keys::Unknown, Keys::Escape, Keys::Unknown, Keys::Unknown,                 /* 25  */
	Keys::Unknown, Keys::Unknown, Keys::Space, Keys::Key1, Keys::Quote,				/* 30  */
	Keys::Backslash, Keys::Key4, Keys::Key5, Keys::Key7, Keys::Quote,              /* 35  */
	Keys::Key9, Keys::Key0, Keys::NumMul, Keys::NumAdd, Keys::Comma,   /* 40  */
	Keys::Minus, Keys::Period, Keys::Slash, Keys::Key0, Keys::Key1,                                 /* 45  */
	Keys::Key2, Keys::Key3, Keys::Key4, Keys::Key5, Keys::Key6,                                              /* 50  */
	Keys::Key7, Keys::Key8, Keys::Key9, Keys::Semicolon, Keys::Semicolon,                                  /* 55  */
	Keys::Comma, Keys::Equals, Keys::Period, Keys::Slash, Keys::Key2,            /* 60  */
	Keys::A, Keys::B, Keys::C, Keys::D, Keys::E,  /* upper case */                            /* 65  */
	Keys::F, Keys::G, Keys::H, Keys::I, Keys::J,                                              /* 70  */
	Keys::K, Keys::L, Keys::M, Keys::N, Keys::O,                                              /* 75  */
	Keys::P, Keys::Q, Keys::R, Keys::S, Keys::T,                                              /* 80  */
	Keys::U, Keys::V, Keys::W, Keys::X, Keys::Y,                                              /* 85  */
	Keys::Z, Keys::SquareBracketLeft, Keys::Backslash, Keys::SquareBracketRight, Keys::Key6, /* 90  */
	Keys::Minus, Keys::Backquote, Keys::A, Keys::B, Keys::C,                             /* 95  */
	Keys::D, Keys::E, Keys::F, Keys::G, Keys::H,  /* lower case */                            /* 100 */
	Keys::I, Keys::J, Keys::K, Keys::L, Keys::M,                                              /* 105 */
	Keys::N, Keys::O, Keys::P, Keys::Q, Keys::R,                                              /* 110 */
	Keys::S, Keys::T, Keys::U, Keys::V, Keys::W,                                              /* 115 */
	Keys::X, Keys::Y, Keys::Z, Keys::SquareBracketLeft, Keys::Backslash,                                   /* 120 */
	Keys::SquareBracketRight, Keys::Backquote, Keys::Backspace                                      /* 125 */
};

struct SpecialKeyCode
{
	const char* str;
	Keys::Enum key;
};

// Some codes for F-keys can differ, depending on whether we are reading a
// /dev/tty from within X or from a text console.
// Some keys (e.g. Home, Delete) have multiple codes;
// one for the standard version and one for the numpad version.
SpecialKeyCode terminalSpecialKeyMap[] =
{
	{ "[A", Keys::Up },
	{ "[B", Keys::Down },
	{ "[C", Keys::Right },
	{ "[D", Keys::Left },

	{ "[E", Keys::Key5 },      // Numpad 5 has no second function - do this to avoid the code being interpreted as Escape.

	{ "OP", Keys::F1 },     /* Within X */
	{ "[[A", Keys::F1 },    /* Text console */
	{ "OQ", Keys::F2 },     /* Within X */
	{ "[[B", Keys::F2 },    /* Text console */
	{ "OR", Keys::F3 },     /* Within X */
	{ "[[C", Keys::F3 },    /* Text console */
	{ "OS", Keys::F4 },     /* Within X */
	{ "[[D", Keys::F4 },    /* Text console */
	{ "[15~", Keys::F5 },   /* Within X */
	{ "[[E", Keys::F5 },    /* Text console */
	{ "[17~", Keys::F6 },
	{ "[18~", Keys::F7 },
	{ "[19~", Keys::F8 },
	{ "[20~", Keys::F9 },
	{ "[21~", Keys::F10 },
	{ "[23~", Keys::F11 },
	{ "[24~", Keys::F12 },

	{ "[1~", Keys::Home },
	{ "OH", Keys::Home },
	{ "[2~", Keys::Insert },
	{ "[3~", Keys::Delete },
	{ "[4~", Keys::End },
	{ "OF", Keys::End },
	{ "[5~", Keys::PageUp },
	{ "[6~", Keys::PageDown },
	{ NULL, Keys::Unknown }
};

static const char* keyboardEventTypes[] =
{
	"released", "pressed", "held"
};

static Keys::Enum keyboardKeyMap[] =
{
	Keys::Unknown, Keys::Escape,
	Keys::Key1, Keys::Key2, Keys::Key3, Keys::Key4, Keys::Key5, Keys::Key6, Keys::Key7, Keys::Key8, Keys::Key9, Keys::Key0, Keys::Minus, Keys::Equals,
	Keys::Backspace, Keys::Tab,
	Keys::Q, Keys::W, Keys::E, Keys::R, Keys::T, Keys::Y, Keys::U, Keys::I, Keys::O, Keys::P,
	Keys::SquareBracketLeft, Keys::SquareBracketRight, Keys::Return, Keys::Control,
	Keys::A, Keys::S, Keys::D, Keys::F, Keys::G, Keys::H, Keys::J, Keys::K, Keys::L, Keys::Semicolon,
	Keys::Quote, Keys::Backquote, Keys::Shift,
	Keys::Backslash, Keys::Z, Keys::X, Keys::C, Keys::V, Keys::B, Keys::N, Keys::M, Keys::Comma, Keys::Period, Keys::Slash,
	Keys::Shift,
	Keys::NumMul,
	Keys::Alt, Keys::Space, Keys::CapsLock,
	Keys::F1, Keys::F2, Keys::F3, Keys::F4, Keys::F5, Keys::F6, Keys::F7, Keys::F8, Keys::F9, Keys::F10,
	Keys::NumLock, Keys::ScrollLock,
	Keys::Num7, Keys::Num8, Keys::Num9,
	Keys::NumSub,
	Keys::Num4, Keys::Num5, Keys::Num6,
	Keys::NumAdd,
	Keys::Num1, Keys::Num2, Keys::Num3, Keys::Num0,
	Keys::NumPeriod,
	Keys::Unknown, Keys::Unknown, Keys::Unknown,
	Keys::F11, Keys::F12,
	Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown,
	Keys::Return, Keys::Control, Keys::NumDiv, Keys::PrintScreen, Keys::Alt, Keys::Unknown,
	Keys::Home, Keys::Up, Keys::PageUp, Keys::Left, Keys::Right, Keys::End, Keys::Down,
	Keys::PageDown, Keys::Insert, Keys::Delete
};

static Keys::Enum keyboardShiftedKeyMap[] =
{
	Keys::Unknown, Keys::Escape,
	Keys::Key1, Keys::Key2, Keys::Backslash, Keys::Key4, Keys::Key5, Keys::Key6, Keys::Key7, Keys::Key8, Keys::Key9, Keys::Key0, Keys::Minus, Keys::Equals,
	Keys::Backspace, Keys::Tab,
	Keys::Q, Keys::W, Keys::E, Keys::R, Keys::T, Keys::Y, Keys::U, Keys::I, Keys::O, Keys::P,
	Keys::SquareBracketLeft, Keys::SquareBracketRight, Keys::Return, Keys::Control,
	Keys::A, Keys::S, Keys::D, Keys::F, Keys::G, Keys::H, Keys::J, Keys::K, Keys::L, Keys::Semicolon,
	Keys::Quote, Keys::Backquote, Keys::Shift,
	Keys::Backslash, Keys::Z, Keys::X, Keys::C, Keys::V, Keys::B, Keys::N, Keys::M, Keys::Comma, Keys::Period, Keys::Slash,
	Keys::Shift,
	Keys::NumMul,
	Keys::Alt, Keys::Space, Keys::CapsLock,
	Keys::F1, Keys::F2, Keys::F3, Keys::F4, Keys::F5, Keys::F6, Keys::F7, Keys::F8, Keys::F9, Keys::F10,
	Keys::NumLock, Keys::ScrollLock,
	Keys::Num7, Keys::Num8, Keys::Num9,
	Keys::NumSub,
	Keys::Num4, Keys::Num5, Keys::Num6,
	Keys::NumAdd,
	Keys::Num1, Keys::Num2, Keys::Num3, Keys::Num0,
	Keys::NumPeriod,
	Keys::Unknown, Keys::Unknown, Keys::Unknown,
	Keys::F11, Keys::F12,
	Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown, Keys::Unknown,
	Keys::Return, Keys::Control, Keys::NumDiv, Keys::PrintScreen, Keys::Alt, Keys::Unknown,
	Keys::Home, Keys::Up, Keys::PageUp, Keys::Left, Keys::Right, Keys::End, Keys::Down,
	Keys::PageDown, Keys::Insert, Keys::Delete
};

Keys::Enum InternalOS::getSpecialKey(Keys::Enum firstCharacter) const
{
	int len = 0, pos = 0;
	unsigned char key, buf[6];

	// Read until we have the full key.
	while ((read(devfd, &key, 1) == 1) && len < 6)
	{
		buf[len++] = key;
	}
	buf[len] = '\0';

	// Find the matching special key from the map.
	while (terminalSpecialKeyMap[pos].str != NULL)
	{
		if (!strcmp(terminalSpecialKeyMap[pos].str, (const char*)buf))
		{
			return (terminalSpecialKeyMap[pos].key);
		}
		pos++;
	}

	if (len > 0)
		// Unrecognised special key read.
	{
		return (Keys::Unknown);
	}
	else
		// No special key read, so must just be the first character.
	{
		return (firstCharacter);
	}
}

Result::Enum ShellOS::handleOSEvents()
{
	// Check user input from the available input devices.

#if defined(__linux__)
	// Terminal.
	if (m_OSImplementation->devfd > 0)
	{
		unsigned char initialKey;
		int bytesRead = read(m_OSImplementation->devfd, &initialKey, 1);
		Keys::Enum key = Keys::Unknown;
		if ((bytesRead > 0) && initialKey)
		{
			// Check for special multi-character key - (first character is Escape or F7).
			if ((terminalStandardKeyMap[initialKey] == Keys::Escape) || (terminalStandardKeyMap[initialKey] == Keys::F7))
			{
				key = m_OSImplementation->getSpecialKey(terminalStandardKeyMap[initialKey]);
			}
			else
			{
				key = terminalStandardKeyMap[initialKey];
			}
		}
		m_shell->onKeyDown(key);
		m_shell->onKeyUp(key);
	}

	// Keyboard.
	if (m_OSImplementation->keyboard_fd > 0)
	{
		struct input_event
		{
			struct timeval time;
			unsigned short type;
			unsigned short code;
			unsigned int value;
		} keyinfo;

		int bytes = read(m_OSImplementation->keyboard_fd, &keyinfo, sizeof(struct input_event));

		if ((bytes == sizeof(struct input_event)) && (keyinfo.type == EV_KEY))
		{
			// Update shift key status.
			if (keyboardKeyMap[keyinfo.code] == Keys::Shift)
			{
				m_OSImplementation->keyboardShiftHeld = (keyinfo.value > 0);
			}

			// Select standard or shifted key map.
			Keys::Enum* keyMap = m_OSImplementation->keyboardShiftHeld ? keyboardShiftedKeyMap : keyboardKeyMap;

			if (keyinfo.value == 0)
			{
				m_shell->onKeyUp(keyMap[keyinfo.code]);
			}
			else
			{
				m_shell->onKeyDown(keyMap[keyinfo.code]);
			}
		}
	}

	// Keypad.
	if (m_OSImplementation->keypad_fd > 0)
	{
		struct input_event
		{
			struct timeval time;
			unsigned short type;
			unsigned short code;
			unsigned int value;
		} keyinfo;

		int bytes = read(m_OSImplementation->keypad_fd, &keyinfo, sizeof(struct input_event));

		if (bytes == sizeof(struct input_event) && keyinfo.type == 0x01)
		{
			Keys::Enum key;
			switch (keyinfo.code)
			{
			case 22: case 64: case 107: key = Keys::Escape; break;// End call button on Zoom2
			//case 37:
			//case 65:
			//{
			//	m_shell->onSystemEvent(SystemEvent::Screenshot);
			//	break;
			//}
			case 28: key = Keys::Space; break; // Old Select
			case 46: case 59: key = Keys::Key1; break;
			case 60: key = Keys::Key2; break;
			case 103: key = Keys::Up; break;
			case 108: key = Keys::Down; break;
			case 105: key = Keys::Left; break;
			case 106: key = Keys::Right; break;
			default: key = Keys::Unknown; break;
			}
			if (key != Keys::Unknown)
			{
				keyinfo.value == 0 ? m_shell->onKeyUp(key) : m_shell->onKeyDown(key);
			}

		}
	}
#elif defined(_WIN32)
	struct SKeyCodeMap
	{
		int key;
		int code;
	};

	static const SKeyCodeMap testKeys[] =
	{
		{ 'q', -1 },
		{ 'Q', -1 },
		{ VK_ESCAPE, -1 },
		{ 's', -2 },
		{ 'S', -2 },
		{ '1', Key1 },
		{ '2', Key2 },
		{ VK_SPACE, KeySpace },
		{ VK_UP, KeyUp },
		{ VK_DOWN, KeyDown },
		{ VK_LEFT, KeyLeft },
		{ VK_RIGHT, KeyRight }
	};

	static const size_t nSizeTestKeys = sizeof(testKeys) / sizeof(SKeyCodeMap);

	static bool pressed[nSizeTestKeys] = { 0 };

	// Scan each of the keys we're interested in.
	for (int i = 0; i < nSizeTestKeys; ++i)
	{
		if (GetAsyncKeyState(testKeys[i].key) & 0x8000)
		{
			if (!pressed[i])
			{
				switch (testKeys[i].code)
				{
				case -1:
				{
					Event e(EventTypeQuit);
					eventManager.submitEvent(e);
				}
				break;
				case -2:
				{
					Event e(EventTypeScreenshot);
					eventManager.submitEvent(e);
				}
				break;
				default:
				{
					Event e(EventTypeKeyDown);
					e.key.key = testKeys[i].code;
					eventManager.submitEvent(e);
					pressed[i] = true;
				}
				break;
				}
			}
		}
		else if (pressed[i])
		{
			Event e(EventTypeKeyUp);
			e.key.key = testKeys[i].code;
			eventManager.submitEvent(e);
			pressed[i] = false;
		}
	}
#endif

	return Result::Success;
}

bool ShellOS::isInitialized()
{
	return m_OSImplementation && m_OSImplementation->isInitialized;
}

Result::Enum ShellOS::popUpMessage(const tchar* title, const tchar* message, ...) const
{
	if (!message)
	{
		return Result::NoData;
	}

	va_list arg;

	va_start(arg, message);
	Log.vaOutput(Log.Information, message, arg);
	va_end(arg);

	return Result::Success;
}
}
}
//!\endcond