include (snp_effect.pri)

# Input
HEADERS += 	src/rsnp_tools/RSnpToolsWorker.h \
			src/snp_toolbox/DamageEffectEvaluator.h \
			src/snp_toolbox/SNPToolboxTask.h \
			src/snp_toolbox/SNPToolboxWorker.h \
            src/snp_toolbox/VariationInfo.h \
			src/tata_box_analysis/AnalyzeTataBoxesTask.h \
			src/tata_box_analysis/AnalyzeTataBoxesWorker.h \
			src/tata_box_analysis/TBP_TATA.h \
			src/BaseRequestForSnpWorker.h \
			src/RequestForSnpTask.h \
			src/SNPEffectPlugin.h \
			src/SNPReportWriter.h \
            src/SNPReportWriterTask.h \
            src/SnpRequestKeys.h
           
SOURCES += 	src/rsnp_tools/RSnpToolsWorker.cpp \
			src/snp_toolbox/DamageEffectEvaluator.cpp \
			src/snp_toolbox/SNPToolboxTask.cpp \
			src/snp_toolbox/SNPToolboxWorker.cpp \
            src/snp_toolbox/VariationInfo.cpp \
			src/tata_box_analysis/AnalyzeTataBoxesTask.cpp \
			src/tata_box_analysis/AnalyzeTataBoxesWorker.cpp \
			src/tata_box_analysis/TBP_TATA.cpp \
			src/BaseRequestForSnpWorker.cpp \
			src/RequestForSnpTask.cpp \
			src/SNPEffectPlugin.cpp \
			src/SNPReportWriter.cpp \
            src/SNPReportWriterTask.cpp

RESOURCES += snp_effect.qrc