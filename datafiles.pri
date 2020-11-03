unix {
	CONFIG(debug, debug|release) {
		QMAKE_CLEAN += $$DEST_DATA_DIR/datafiles_installed
		POST_TARGETDEPS += $$DEST_DATA_DIR/datafiles_installed
		QMAKE_EXTRA_TARGETS += $$DEST_DATA_DIR/datafiles_installed
		$$DEST_DATA_DIR/datafiles_installed.commands = \
			mkdir -p $$DEST_DATA_DIR \
			&& ln -sf $$SRC_DATA_DIR/* $$DEST_DATA_DIR \
			&& touch $$DEST_DATA_DIR/datafiles_installed
	}
	else {
		POST_TARGETDEPS += datafiles
		QMAKE_EXTRA_TARGETS += datafiles
		datafiles.commands = \
			mkdir -p $$DEST_DATA_DIR \
			&& rsync -r $$SRC_DATA_DIR/ $$DEST_DATA_DIR
	}
}
win32 {
	POST_TARGETDEPS += datafiles
	QMAKE_EXTRA_TARGETS += datafiles
	datafiles.commands = \
		xcopy $$system_quote($$system_path($$SRC_DATA_DIR)) $$system_quote($$system_path($$DEST_DATA_DIR)) /s /e /y /i
}
