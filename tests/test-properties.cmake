#### Expected Failures
# Test cases that test some aspect of the Mac API that Executor does not currently support,
# but which are not cause for immediate concern.

set_tests_properties(
        FileTest.SetFInfo_CrDat 
        FileTest.SetFLock 
        FileTest.MakeFSSpec
        QuickDraw.PaintCircle
        QuickDraw.FrameCircle1
        QuickDraw.FrameRoundRect
    APPEND PROPERTIES LABELS xfail)
