# WINdowsTOUCHGestures

A Windows Application that enables 4+ finger Multi-Touch-Gestures for controlling applications on windows.


## Table of contents
- [Features](#Features)
	- [Quick Control Panel](#Quick_Control_Panel)
	- [Application Switcher](#Application_Switcher)
- [Targeted Systems](#Targeted_Systems)
- [Manual Building](#Manual_Building)
- [Uninstalling](#Uninstalling)
- [Inner Workings](#Inner_Workings)



## Features
<a name="Features"></a>

- Gestures are started when 4+ touch points are recognized
- Gestures are finished when 0-1 touch points remain
	- Depending on gesture, a gesture will either result in:
		- An action being taken when the gesture progressed sufficiently far
		- No action being taken when the the gesture ends around where it started
- Animations played during gestures don't play on their own. Instead, their progress and speed is entirely dictated by your fingers
	- e.g.: if you start zooming in on a non-maximized window, then the displayed animation grows/shrinks the window as you spread/contract your fingers.
	- All animations are done via DWM Thumbnails
		- The contents of windows visible in animations continue to update in real-time
		- All of the rendering done by windows itself
			- The overhead isn't any greater than you hovering over the task bar and looking at the previews it shows for windows
- Gestures behave differently based on being used on:
	- A maximized window
	- A non-maximized window
	- The desktop window (iow: background)
- Recognized gestures are:
	- Horizontal Swiping within a maximized window:
		- If there are at least 2 windows that would also appear on the task-bar:
			- Animation:
				- Move the current window towards the left/right, and make it opaque
				- Bring in the prev/next window (according to an internal order of windows) from the right/left
				- The window brought in will become less opaque the closer it gets towards the center
			- Action:
				- The original window is minimized (if it has a minimize-icon) (`ShowWindow(hWnd, SW_MINIMIZE);`)
				- The next window is shown (preferrably as maximized)
				- The next window is set as the foreground and active window (`SetForegroundWindow(hNext); SetActiveWindow(hNext);`)
		- Otherwise:
			- Animation: Move the window a bit to the left/right, but snap back when the gesture ends
			- Action: Nothing
	- Swiping down in a maximized window:
		- Animation: The quick control panel will move in from the top of the screen (see below)
		- Action: Display the quick control panel
	- Swiping up in a maximized window:
		- Animation:
			- Animate the opening of an application switcher for all of the windows also considered by the maximized-window-horizontal-swipe gesture
			- If the switcher is activated from within a maximized window, that window will be animated as it shrinks towards its eventual position as that position would be displayed within the application switcher
		- Action: Activate the application switcher
	- Zooming out (pinch) of a maximized window:
		- If the window can be resotred (has a maximize-icon in its title-bar)
			- Animation:
				- Strech the window towards its eventual position after being maximized
				- A blur overlay is created behind the window being stretched.
					- The effect becomes stronger as the window becomes larger, and is non-existant at the window's original scale
			- Action: The window is restored (iow: will no longer be maximized) (`ShowWindow(hWnd, SW_RESTORE);`)
		- Otherwise:
			- Animation: Shrink the window a bit, but snap back once the gesture ends
			- Action: Nothing
	- Swiping (any direction) in a non-maximized window:
		- If the window is movable (has a title bar):
			- Animation: The window moves together with your finger
			- Action: The window is moved to the new position (`SetWindowPos(...)`)
		- Otherwise (isn't supposed to be movable; e.g. the start menu):
			- Animation:
				- The window is dragged for a couple of pixels, with diminishing returns, before jumping back when the gesture ends.
				- A blur effect exists behind the window being moved, that becomes stronger the further the window is displayed from its original position
			- Action: Nothing
	- Zooming in on a non-maximized window:
		- If the window can be maximized (has a maximize-icon in its title-bar)
			- Animation: The inverse of the maximized-window-zoom-out animation
			- Action: Maximize the window (`ShowWindow(hWnd, SW_MAXIMIZE);`)
		- Otherwise:
			- Animation: Stretch the window a bit, but snap back once the gesture ends
			- Action: Nothing
	- Zooming out (pinch) of a non-maximized window:
		- If the window can be minimized (has a minimize-icon in its title-bar)
			- Animation: The window shrinks towards its center and becomes more and more opaque the smaller it gets
			- Action: The window will be remembered for the next zoom-in-on-desktop gesture, and the window is minimized (`ShowWindow(hWnd, SW_MINIMIZE);`)
		- Otherwise:
			- Animation: Shrink the window a bit, but snap back once the gesture ends
			- Action: Nothing
	- Vertical swiping on the desktop background:
		- Same as it for maximized windows
	- Horizontal swiping on the desktop background:
		- Same as it for non-movable window, allowing you to jiggle around your desktop icons without that actually doing anything :)
	- Zoom in on the desktop background:
		- If the last window that was minimized by the non-maximized-window-zoom-out gesture still exists, and is still minimized:
			- Animation: The inverse of the animation used for minimizing a window
			- Action: Restore the previously minimized window to its original position (`ShowWindow(hWnd, SW_RESTORE);`)
		- Otherwise:
			- Animation: Just like the Zoom-out animation for the desktop
			- Action: Nothing
	- Zoom out of the desktop background:
		- Animation: Blur the background and let the desktop icons move a tiny bit alongside the gesture, before snapping back
		- Action: Nothing


### Quick Control Panel
<a name="Quick_Control_Panel"></a>

- Contains a number of useful buttons & sliders for controlling your PC:
	- Sliders for display brightness and volume
	- An analogue and digital clock, together with a date display
	- Toggle-buttons for:
		- WiFi
		- Bluetooth
		- Rotation Lock
		- Standby control (tri-state: allow standby, prevent standby, keep screen on)
			- Also displays keep-alive state as set by other programs, and the delay before machine will enter standby mode (if currently possible)
	- Planned buttons: toggle Flight-Mode, Night-Mode, Performance/Powersaving
- Disappears once it looses focus
- Doesn't exist, and isn't hooked into anything while not visible (meaning that it won't affect system performance while not shown)


### Application Switcher
<a name="Application_Switcher"></a>

- ... has yet to be programmed (TODO)



## Targeted Systems
<a name="Targeted_Systems"></a>

- Wintouchg is only meant to be used for multi-touch Touch displays. Attempting to use it with pen displays isn't intended, and mouse input doesn't do anything at all.
- Developed for, and working best on:
	- 13.9' display
	- 1080p resolusion (1920x1080)
	- 125% DPI scaling
- Other configurations should also work, but may not feel as good, as gesture start/finish threshold, and zoom speed factors must be hard-coded in pixels, meaning that the actual, physical DPI of your device will affect your expiriece with this program


## Build+Install
<a name="Build_Install"></a>

This project only contains 2 source files `main.c` and `gui/gui.c`, the later file being auto-generated for the purpose of containing RGBA-buffers for the elements that make up the control panel and application switcher.

As such, there are many ways to build this project, but because of some caveats that relate to this application needing to make use of `RegisterPointerInputTarget`, the build process becomes quite a bit more complicated.

If your set-up matches mine (have cygwin installed, and all the necessary tools (or batch-file shortcuts for all of them) located in `%PATH%`), all you really need to do is:

- Run this once (this'll install a self-signing certificate that is needed for building `wintouchg.exe` that you will have to acknowledge):
	- `make setup`
- Run this whenever you want to build `wintouchg.exe`:
	- `make`
- Make sure there weren't any errors (if there were any, `wintouchg.exe` might have still been created, but wasn't signed correct)
- Copy `wintouchg.exe` somewhere into `C:\Program Files\...`, unless you've just put the project folder itself there to simplify stuff (like I did)
	- This is required because of [this](https://docs.microsoft.com/en-us/windows/security/threat-protection/security-policy-settings/user-account-control-only-elevate-uiaccess-applications-that-are-installed-in-secure-locations) which essentially prevents _UI_ _Access_ _privilege_ from working for programs not installed in in special locations (see below)
- Launch the copy `C:\Program Files\...` (or possibly set-up a scheduled task to launch it automatically on startup) and enjoy

Note: The Certificate, signing, and strange install location is required because [`RegisterPointerInputTarget()`](https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerpointerinputtarget) (which we need to intercept touch input) requires the calling program to have _UI_ _Access_ _privilege_.



### Manual Building
<a name="Manual_Building"></a>

To manually build wintouchg (i.e. when your set-up differs from mine), follow these steps:

Create the certificate that will be used for signing `wintouchg.exe`:

- ```sh
  makecert.exe -r -pe -n "CN=wintouchg.exe Certificate" -ss PrivateCertStore app-wintouchg.cer
  ```

- For this purpose, search for [`makecert.exe`](https://docs.microsoft.com/en-us/windows/win32/seccrypto/makecert) in `C:\Program Files (x86)\Microsoft SDKs\` and `C:\Program Files\Microsoft SDKs\`
- In my case, it was located at: `"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin\x64\makecert.exe"`
- If you're having troubles, the [question's already been asked](https://stackoverflow.com/questions/51418366/makecert-exe-missing-in-windows-10-how-to-get-it-and-use-it)
- This will create a file `app-wintouchg.cer` that represents the certificate that will be used to sign `wintouchg.exe`


Install the newly created certificate as a _Trusted_ _Root_ _Certificate_. Anything less than that can't be used for signing applications such that they're accepted on the local machine. As such, if you're worried about "_Viruses_", you should delete this file after you've signed `wintouchg.exe` with it, as it could be used to sign anything else, too.

```sh
CertMgr.exe -add app-wintouchg.cer -s -r currentUser root
```

- For this purpose, `CertMgr.exe` should be in the same folder that you've also found `makecert.exe` in (see previous step). If not, search for it in `"C:\Program Files (x86)\Microsoft SDKs\Windows\`
- In my case, it was located at: `"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin\x64\CertMgr.Exe"`
- This command does the actual job of registering the certificate as usable for signing applications. As such, running it will also prompt you to confirm that you really want to install the certificate.
	- Though I might mention that on my machine, this confirmation window isn't one of those UAC-darkened-background-with-yes-no-confirmation-box-in-foreground deals, but rather just a regular old dialog box that you could probably also interact with programmatially, meaning that if you're still worried about "_Viruses_" hi-jacking the certificate, know that if a no-good-person wanted to do this, all you'd see is a window pop up and disappear within a fraction of a second...


Now actually build the source files for Wintouchg:

```sh
gcc -g -c -o gui/gui.o gui/gui.c
gcc -g -c -o main.o main.c
windres -F pe-x86-64 manifest.rc -o manifest.o
gcc -g -o wintouchg.exe main.o manifest.o gui/gui.o -lgdi32
```

- Any C compiler should be usable in place of `gcc`.
- The `windres` program is a utility for compiling windows resource files into object files, and (in my case) comes with cygwin. But note that if your machine is 32-bit, you'll need to use `pe-i386` instead of `pe-x86-64`.
- Other than that, this should be pretty self-explainatory: just your typical build process, only that there's not really any external dependencies other than native window libraries and a minimal Standard C library (nothing really used beyond `printf`, `malloc`, `memcpy`)


Now you must sign the produced executable using the certificate from above:

```sh
signtool.exe sign /v /s PrivateCertStore /n "wintouchg.exe Certificate" wintouchg.exe
```

- For this purpose, search for [`signtool.exe`](https://docs.microsoft.com/en-us/dotnet/framework/tools/signtool-exe) in `C:\Program Files (x86)\Windows Kits\`
- In my case, it was located at: `"C:\Program Files (x86)\Windows Kits\8.1\bin\x64\signtool.exe"`
- If you're having troubles, make sure you've installed the Windows SDK, but the [question's already been asked](https://stackoverflow.com/questions/48257764/where-is-the-signtool-exe-located-in-windows-10-sdk)


If that last step worked, all that's left is to copy `wintouchg.exe` into one of the following locations, before launching it from there:

- `C:\Program Files\` (and all sub-directories)
- `C:\Program Files (x86)\` (and all sub-directories)
- `C:\Windows\system32\` (and all sub-directories)
- This is required because windows only honors the `uiAccess='true'` from the manifest if the associated application is located somewhere in these paths. (s.a. https://docs.microsoft.com/en-us/windows/security/threat-protection/security-policy-settings/user-account-control-only-elevate-uiaccess-applications-that-are-installed-in-secure-locations)

And you're done!


### Uninstalling
<a name="Uninstalling"></a>

Because building+using wintouchg requires creating and registering a new, self-signing certificate on the local machine, this will also have to be uninstalled.

For this purpose, you can start by deleting `wintouchg.exe` from wherever you put it.

In order to delete the certificate, do the following:

- Open a Commandline as Administrator (yes: (fully) deleting a certificate requires admin, but installing it doesn't; don't ask me why (or maybe theres a different way that doesn't require admin; I don't know...))

  ```sh
  CertMgr.exe
  ```

  This is the same `CertMgr.exe` that already took apart in the build process
- This would cause a new window to pop up.
- Inside, navigate to `Trusted Root Certificates` (or whatever it's called in english; german machine here, where it's called `Vertrauenswürdige Stammzertifizierungsstellen`)
- There should be `1` sub-element `Certficates` (`Zertifikate`)
- Click that element and the right-hand window should show a long list of installed root certificates.
- Scroll to the very bottom, where there should be a Certificate `wintouchg.exe Certificate`
- Right-click and delete that one
- Next, in the left pane, navigate to an element `PrivateCertStore`, and its sub-element `Certficates` (`Zertifikate`)
- Inside should be yet another `wintouchg.exe Certificate`, which you can also delete

And you're done!




## Inner Workings
<a name="Inner_Workings"></a>

- By hooking `RegisterPointerInputTarget`, it is possible to intercept touch input, allowing for further processing and special behavior to be done for certain gestures.
- The blur effects are acomplished by making using of the windows 10 blur or windows 7 glass
- Modified input can then be re-introduced via `InjectTouchInput`
- All window move/resize animations are implemented via DWM Thumbnails (Taskbar previews) (`DwmRegisterThumbnail`)
	- As such, windows will not receive resize/move (or any other) messages until the gesture is completed.
- When no gesture is in progress:
	- All touch information is re-injected (as accurately as possible)
- While a gesture is in progress:
	- Touch input send to applications will (seemingly) pause. Wintouchg will just keep on sending the same touch information from before the gesture was started
- When a gesture ends (0-1 touch points remain):
	- New touch inputs (i.e. ones started during the gesture) will be marked as being ignored, and will never be send to applications
	- If the gesture did nothing (i.e. the user began maximizing a window, but zoomed back to around the window's original size):
		- Touch inputs that ended during the gesture are marked as canceled
		- Touch inputs that existed both before and after the gesture are simply resumed
	- If the gesture did something:
		- All touch inputs that were paused for the duration of the gesture are canceled
		- All remaning touch inputs are marked as ignored



