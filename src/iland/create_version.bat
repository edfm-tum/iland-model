@for /f %%i in ('svnversion .') do @Set mysvn=%%i
@echo const char *version = "Alpha 1.1.0";
@echo.const char *svn_revision = "%mysvn%";
@echo const char *currentVersion(){ return version;}
@echo const char *svnRevision(){ return svn_revision;}
