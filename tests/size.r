#include "Processes.r"

resource 'SIZE' (-1) {
	dontSaveScreen,
	ignoreSuspendResumeEvents,
	enableOptionSwitch,
	cannotBackground,
	needsActivateOnFGSwitch,
	backgroundAndForeground,
	dontGetFrontClicks,
	ignoreChildDiedEvents,
	is32BitCompatible,
	reserved,
	reserved,
	reserved,
	reserved,
	reserved,
	reserved,
	reserved,
	2048 * 1024,
	2048 * 1024
};
