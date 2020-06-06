on run argv
	set imageName to item 1 of argv
	tell application "Finder"
		tell disk imageName
			open
			set current view of container window to icon view
			set theViewOptions to the icon view options of container window
			set arrangement of theViewOptions to not arranged
			set icon size of theViewOptions to 128
			tell container window
				set sidebar width to 0
				set statusbar visible to false
				set toolbar visible to false
				set the bounds to {400, 100, 880, 400}
				set position of item "Executor 2000.app" to {135, 125}
				set position of item "Applications" to {345, 125}
			end tell
			close
		end tell
	end tell
end run
