/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2016 UniPro <ugene@unipro.ru>
 * http://ugene.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "AssemblyModel.h"
#include "CoverageInfo.h"

#include <U2Core/U2OpStatusUtils.h>
#include <U2Core/U2SafePoints.h>

#include <algorithm>
#include <limits>

namespace U2 {

void CoverageInfo::updateStats() {
    maxCoverage = 0;
    minCoverage = std::numeric_limits<qint64>::max();

    qint64 sum = 0;

    for(int regionIndex = 0; regionIndex < coverageInfo.size(); ++regionIndex) {
        maxCoverage = std::max(maxCoverage, coverageInfo[regionIndex]);
        minCoverage = std::min(maxCoverage, coverageInfo[regionIndex]);
        sum += coverageInfo[regionIndex];
    }
    averageCoverage = (double)sum/coverageInfo.size();
}

CalcCoverageInfoTask::CalcCoverageInfoTask(const CalcCoverageInfoTaskSettings & settings_) :
BackgroundTask<CoverageInfo>("Calculate assembly coverage", TaskFlags_NR_FOSE_COSC), settings(settings_), calculateTask(NULL) {}


void CalcCoverageInfoTask::prepare() {
    U2AssemblyCoverageStat cachedCoverageStat;
    {
        cachedCoverageStat = settings.model->getCoverageStat(stateInfo);
        if(stateInfo.isCoR()) {
            return;
        }
    }

    qint64 modelLength = 0;
    {
        U2OpStatusImpl status;
        modelLength = settings.model->getModelLength(status);
        if(status.hasError()) {
            stateInfo.setError(status.getError());
            return;
        }
    }
    double basesPerRegion = (double)settings.visibleRange.length/settings.regions;
    double coverageStatBasesPerRegion = (double)modelLength/cachedCoverageStat.coverage->size();

    result.coverageInfo.resize(settings.regions);
    result.region = settings.visibleRange;

    if(cachedCoverageStat.coverage->isEmpty() || (coverageStatBasesPerRegion > basesPerRegion)) {
        calculateTask = new CalculateCoveragePerBaseOnRegionTask(settings.model->getDbiConnection().dbi->getDbiRef(), settings.model->getAssembly().id, settings.visibleRange);
        addSubTask(calculateTask);
    } else {
        for(int regionIndex = 0;regionIndex < settings.regions;regionIndex++) {
            int startPosition = qRound((settings.visibleRange.startPos + basesPerRegion*regionIndex)/coverageStatBasesPerRegion);
            int endPosition = qRound((settings.visibleRange.startPos + basesPerRegion*(regionIndex + 1))/coverageStatBasesPerRegion);
            result.coverageInfo[regionIndex] = 0;
            for(int i = startPosition;i < endPosition;i++) {
                result.coverageInfo[regionIndex] = std::max(result.coverageInfo[regionIndex], (qint64)cachedCoverageStat.coverage->at(i).maxValue);
            }
            result.updateStats();
        }
    }
}

QList<Task*> CalcCoverageInfoTask::onSubTaskFinished(Task* subTask) {
    QList<Task*> tasks;
    CHECK(!subTask->hasError(), tasks);
    CHECK(!hasError(), tasks);
    if (subTask == calculateTask) {
        QVector<CoveragePerBaseInfo> *info = calculateTask->takeResult();
        for (int i = 0; i < info->size(); i++) {
            result.coverageInfo[i] = info->at(i).coverage;
        }

        delete info;
        result.updateStats();
    }
    return tasks;
}

}
