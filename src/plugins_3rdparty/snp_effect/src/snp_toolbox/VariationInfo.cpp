#include "VariationInfo.h"

#include <U2Core/S3TablesUtils.h>
#include <U2Core/VariationPropertiesUtils.h>

#include <U2Formats/Database.h>
#include <U2Formats/GenbankLocationParser.h>

#include <U2Core/U2SequenceDbi.h>
#include <U2Core/U2SafePoints.h>
#include <U2Core/U2OpStatusUtils.h>
#include <U2Core/AppContext.h>
#include <U2Core/AppSettings.h>

#include <U2Core/Timer.h>

#include <algorithm>

namespace U2 {

#define SPLICE_SITE_LEN 2

//////////////////////////////////////////////////////////////////////////
//VariationInfo
VariationInfo::VariationInfo( const U2Variant& var, const U2DataId& _seqId, U2SequenceDbi* _seqDbi, const QString& _sequenceName)
:variant(var)
,seqId(_seqId)
,sequenceName(_sequenceName)
,genesFound(false)
,effectLoaded(false)
,seqDbi(_seqDbi)
{

}

VariationInfo::VariationInfo( const U2Variant& var, const U2DataId& _seqId, const QList<Gene>& _genes, U2SequenceDbi* _seqDbi, const QString& _sequenceName)
:variant(var)
,seqId(_seqId)
,sequenceName(_sequenceName)
,genes(_genes)
,genesFound(true)
,effectLoaded(false)
,seqDbi(_seqDbi)
{

}

void VariationInfo::initInfo(U2VariantDbi* varDbi, QSharedPointer<DamageEffectEvaluator> _evaluator, bool withEffect){
    evaluator = _evaluator;

    if (evaluator.isNull()){
        return;
    }

    U2OpStatusImpl opStatus;
    
    SAFE_POINT(varDbi != NULL, "No variant dbi", );

    variantTrack = varDbi->getVariantTrackofVariant(variant.id, opStatus);
    CHECK_OP(opStatus, )
    
    
    if (!genesFound){
        genes = evaluator->findGenes(variant, seqId, opStatus);
        CHECK_OP(opStatus, );

        genesFound = true;
    }
    if(!effectLoaded && withEffect){
        deffectList = evaluator->getDamageEffect(variant, variantTrack, DamageEffectFull, opStatus);
        CHECK_OP(opStatus, );
        effectLoaded = true;
    }
}

QString VariationInfo::getInGeneEffectInfo( const QString& geneName, QMap<ReportColumns, QString>& rawData ){
    QString result;
    SAFE_POINT(seqDbi != NULL, "Sequence DBI is NULL", QString("Effect is not evaluated\n"));

    U2OpStatusImpl os;
    QByteArray damagedTriplet;
    int aaPos = -1;
    int nuclPos = -1;
    int codonPos = -1;

    if(!containsGene(geneName)){
        result = tr("Gene is not affected");
        return result;
    }
    Gene gene = getGeneByName(geneName);
    
    if(!effectLoaded && !evaluator.isNull() ){
        U2OpStatusImpl opStatus;
        evaluator->getDamageEffect(variant, variantTrack, DamageEffectFull, opStatus);
        CHECK_OP(opStatus, QString("Effect is not evaluated\n"));
        effectLoaded = true;
    }
     
    bool hasEffect = false;
    bool hasKnownEffect = false;
    DamageEffect effect;
    DamageEffect knownEffect;
    foreach(const DamageEffect& de, deffectList){
        if(de.affectedGeneName.compare(gene.getName(), Qt::CaseInsensitive) == 0){
            effect = de;
            hasEffect = true;
            break;
        }
        if (de.affectedGeneName.isEmpty()){
            knownEffect = de;
            hasKnownEffect = true;
        }
    }

    bool canHaveEffect = true;
    if ((!VARIATION_REGION(variant).intersects(gene.getCDSRegion())
        && -1 == VARIATION_REGION(variant).findIntersectedRegion(gene.getExons())
        && !VariationPropertiesUtils::isInSpliceSite(gene.getExons(), variant.startPos, SPLICE_SITE_LEN, gene.isComplemented()))
        || gene.getName().startsWith("FR"))
    {
        canHaveEffect = false;
    }

    result.append(QString("%1 --> %2").arg(QString::fromAscii(variant.refData)).arg(QString::fromAscii(variant.obsData)));
    U2Region variantRegion (variant.startPos, 1);
    QByteArray refDataInDb = seqDbi->getSequenceData(seqId, variantRegion, os);
    refDataInDb = refDataInDb.toUpper();

    if(refDataInDb != variant.refData){ //change SNP ref data if needed
        result.append(QString("    WARNING. Reference data: %1").arg(QString::fromAscii(refDataInDb)));
    }
    result.append("\n");
    if(canHaveEffect){
     
        if(VariationPropertiesUtils::isDamageProtein(variant, gene) && gene.isCoding()){
            if(!VariationPropertiesUtils::getFrameStartPositionsForCoding(&nuclPos, &aaPos, &codonPos, variant, gene)){
                return result;
            }

            result.append(QString("Position in protein: %1\n").arg(aaPos + 1));

            damagedTriplet = VariationPropertiesUtils::getDamagedTriplet(gene, nuclPos, seqId, seqDbi, os);
            damagedTriplet = damagedTriplet.toUpper();
            QByteArray variedTriplet;

            DNAAlphabet* alphabet = AppContext::getDNAAlphabetRegistry()->findById(BaseDNAAlphabetIds::NUCL_DNA_DEFAULT());
            SAFE_POINT(alphabet != NULL, "No Alphabet", result);

            if (gene.isComplemented()){

                QList<DNATranslation*> translList = AppContext::getDNATranslationRegistry()->lookupTranslation(alphabet, DNATranslationType_NUCL_2_COMPLNUCL);
                SAFE_POINT(!translList.isEmpty(), "No compl Translations", result);
                DNATranslation* complTransl =  translList.first();

                complTransl->translate(damagedTriplet.data(), damagedTriplet.size());

                variedTriplet = VariationPropertiesUtils::varyTriplet(damagedTriplet, variant, codonPos, complTransl);

            }else{
                variedTriplet = VariationPropertiesUtils::varyTriplet(damagedTriplet, variant, codonPos);
            }
            result.append(QString("Codon: %1 => %2\n").arg(QString::fromAscii(damagedTriplet)).arg(QString::fromAscii(variedTriplet)));
           
            QList<DNATranslation*> translList = AppContext::getDNATranslationRegistry()->lookupTranslation(alphabet, DNATranslationType_NUCL_2_AMINO);
            SAFE_POINT(!translList.isEmpty(), "No Translations", result);
            DNATranslation* aaTransl =  translList.first();
            char damageTransl = aaTransl->translate3to1(damagedTriplet[0], damagedTriplet[1], damagedTriplet[2]);
            char variedTransl = aaTransl->translate3to1(variedTriplet[0], variedTriplet[1], variedTriplet[2]);
            result.append(QString("Translation: %1 => %2\n").arg(damageTransl).arg(variedTransl));

            if (damageTransl == variedTransl){
                result.append(QString("Synonymous\n"));
            }else{
                result.append(QString("Non-synonymous\n"));
            }

            //improved SIFT SCORE
            if(!hasEffect || effect.siftEffectValue == DAMAGE_EFFECT_UNDEFINED_SCORE){
                if(!gene.isCoding()){
                    result.append("Non-protein coding\n");
                }else{
                    result.append("Tolerance Score (SIFT): Not calculated\n");
                }
            }else{
                result.append(QString("Tolerance Score (SIFT): %1").arg(effect.siftEffectValue));

                //VariationRelatedSettings* vrs = AppContext::getAppSettings()->getVariationRelatedSettings();
                //if (vrs){
                    float threshold = 0.05f;
                    if(effect.siftEffectValue < threshold){
                        result.append(" (DAMAGING)");
                    }else{
                        result.append(" (TOLERATED)");
                    }
                //}
                result.append("\n");

            }

          

            CHECK_OP(os, result);
        }

    }
    if(hasKnownEffect){
        if (!knownEffect.discardFilter.isEmpty()){
            result.append(QString("Known TOLERATED. Discard reason: %1\n").arg(knownEffect.discardFilter));
        }else{
            result.append(QString("Known DAMAGING effect\n"));
        }

        //known scores
        if(knownEffect.avSift != DAMAGE_EFFECT_UNDEFINED_SCORE){
            result.append(QString("AV SIFT: %1\n").arg(knownEffect.avSift));
        }
        if(knownEffect.ljb_lrt != DAMAGE_EFFECT_UNDEFINED_SCORE){
            result.append(QString("LRT: %1\n").arg(knownEffect.ljb_lrt));
        }
        if(knownEffect.ljb_phylop != DAMAGE_EFFECT_UNDEFINED_SCORE){
            result.append(QString("PhyloP: %1\n").arg(knownEffect.ljb_phylop));
        }
        if(knownEffect.ljb_pp2 != DAMAGE_EFFECT_UNDEFINED_SCORE){
            result.append(QString("PolyPhen2: %1\n").arg(knownEffect.ljb_pp2));
        }
        if(knownEffect.ljb_mt != DAMAGE_EFFECT_UNDEFINED_SCORE){
            result.append(QString("MutationTaster: %1\n").arg(knownEffect.ljb_mt));
        }
        if(knownEffect.genomes1000 != DAMAGE_EFFECT_UNDEFINED_SCORE){
            result.append(QString("1000 genomes allele freq: %1\n").arg(knownEffect.genomes1000));
        }
        if(knownEffect.segmentalDuplication){
            result.append(QString("In segmental duplication region\n"));
        }
        if(knownEffect.conserved){
            result.append(QString("In conserved region\n"));
        }
        if(knownEffect.gerpConcerved != DAMAGE_EFFECT_UNDEFINED_SCORE){
            result.append(QString("GERP Conserved region: %1\n").arg(knownEffect.gerpConcerved));
        }
        if(knownEffect.allFreq != DAMAGE_EFFECT_UNDEFINED_SCORE){
            result.append(QString("Alternative allele frequency : %1\n").arg(knownEffect.allFreq));
        }
        if(knownEffect.hapmap != DAMAGE_EFFECT_UNDEFINED_SCORE){
            result.append(QString("Scores of haplotype map: %1\n").arg(knownEffect.hapmap));
        }
        if(knownEffect.gerpScore != DAMAGE_EFFECT_UNDEFINED_SCORE){
            result.append(QString("GERP Score: %1\n").arg(knownEffect.gerpScore));
        }
    }
    return result;
}

QString VariationInfo::getInGeneLocationInfo( const QString& geneName, QMap<ReportColumns, QString>& rawData ){
    QString result;
    if(!containsGene(geneName)){
        result = tr("Out of gene");
        return result;
    }
    Gene gene = getGeneByName(geneName);
    if (!gene.getRegion().intersects(VARIATION_REGION(variant))){
        result = tr("Out of gene");
        return result;
    }else if(gene.getCDSRegion().intersects(VARIATION_REGION(variant))){  //in CDS
        if (!gene.getName().startsWith("FR")){
            result += tr("CDS. ");
        }
        
        QVector<U2Region> exons = gene.getExons();

        //exons has been sorted
       //qStableSort(exons.begin(), exons.end()); //sort by region start pos first
//         if (gene.isComplemented()){
//             std::reverse(exons.begin(), exons.end());
//         }

        int exonIdx = VARIATION_REGION(variant).findIntersectedRegion(exons);
        if(exonIdx == -1){ //not in exon
            U2Region nearestExont = U2_REGION_MAX;
            bool isDonor= false;
            U2Region reg = VariationPropertiesUtils::getIntron(gene.getRegion(), exons, variant.startPos, nearestExont, isDonor);
            if (reg != U2_REGION_MAX){
                if (nearestExont != U2_REGION_MAX){
                    if (gene.isComplemented()){
                        isDonor = !isDonor;
                    }
                    result += tr("%1 splice-site of the intron %2, nearest exon is: %3")
                        .arg(isDonor ? "Donor" : "Acceptor")
                        .arg(Genbank::LocationParser::buildLocationString(QVector<U2Region>()<<reg))
                        .arg(Genbank::LocationParser::buildLocationString(QVector<U2Region>()<<nearestExont));
                }else{
                    result += tr("Intron: %1").arg(Genbank::LocationParser::buildLocationString(QVector<U2Region>()<<reg));
                }
            }else{
                result += tr("Intron.");
            }
        }else{ //in exon
            result += tr("Exon: %1").arg(Genbank::LocationParser::buildLocationString(QVector<U2Region>()<<exons.at(exonIdx)));

            //check if in splice site
            int spliceSiteLen = SPLICE_SITE_LEN;
            bool isDonor = false;
            bool isAcceptor = false;
            int exonsSize = exons.size();
            for(int i = 0; i < exons.size(); i++){
                const U2Region& reg = exons.at(i);
                if(( (i!=0 || gene.isComplemented()) &&  (i != exonsSize - 1 || !gene.isComplemented()) )
                    && (VariationPropertiesUtils::isInAcceptorSpliceSite(reg, variant.startPos, spliceSiteLen)) ){
                    isAcceptor = true;
                    break;
                }
                if ((( (i!=0 || !gene.isComplemented()) &&  (i != exonsSize - 1 || gene.isComplemented()) )
                    && VariationPropertiesUtils::isInDonorSpliceSite(reg, variant.startPos, spliceSiteLen)) ){
                    isDonor = true;
                    break;
                }
            }
            if (isDonor){
                if (gene.isComplemented()){
                    isDonor = !isDonor;
                }
                result += tr(". %1 splice-site.").arg(isDonor ? "Donor" : "Acceptor");
            }
            if (isAcceptor){
                if (gene.isComplemented()){
                    isAcceptor = !isAcceptor;
                }
                result += tr(". %1 splice-site.").arg(isAcceptor ? "Acceptor" : "Donor");
            }
        }

    }else{ //out of CDS
        if (!gene.getName().startsWith("FR")){
            result += tr("Out of CDS. ");
        }

        QVector<U2Region> exons = gene.getExons();

        //exons has been sorted
        //qStableSort(exons.begin(), exons.end()); //sort by region start pos first
//         if (gene.isComplemented()){
//             std::reverse(exons.begin(), exons.end());
//         }

        bool onGene5End = false;
        bool complitelyOut = true;
        int exonIdx = VARIATION_REGION(variant).findIntersectedRegion(exons);
        if (exonIdx == -1){
            U2Region nearestExont = U2_REGION_MAX;
            bool isDonor= false;
            U2Region reg = VariationPropertiesUtils::getIntron(gene.getRegion(), exons, variant.startPos, nearestExont, isDonor);
            if(variant.startPos < gene.getCDSRegion().startPos){ // left to gene
                onGene5End = true;
            }else{ //right to gene
                onGene5End = false;
            }
            if(gene.isComplemented()){
                onGene5End = !onGene5End;
                isDonor = !isDonor;
            }
            result += tr("%1'-end of gene. ").arg(onGene5End ? 5 : 3);
            if (nearestExont!= U2_REGION_MAX){
                if (gene.isComplemented()){
                    isDonor = !isDonor;
                }
                result += tr("%1 splice-site of the intron %2, nearest exon is: %3")
                    .arg(isDonor ? "Donor" : "Acceptor")
                    .arg(Genbank::LocationParser::buildLocationString(QVector<U2Region>()<<reg))
                    .arg(Genbank::LocationParser::buildLocationString(QVector<U2Region>()<<nearestExont));
            }else{
                if (reg != U2_REGION_MAX){
                    result += tr("Intron: %1").arg(Genbank::LocationParser::buildLocationString(QVector<U2Region>()<<reg));
                }else{
                    result += tr("Intron.");
                }
            }
        }else{
            const U2Region& exon = exons.at(exonIdx);
            if(variant.startPos < gene.getCDSRegion().startPos){ // left to gene
                onGene5End = true;
                if(exon.intersects(gene.getCDSRegion())){ //partially out
                    complitelyOut = false;
                }
            }else{ //right to gene
                onGene5End = false;
                if(exon.intersects(gene.getCDSRegion())){ //partially out
                    complitelyOut = false;
                }
            }
            if(gene.isComplemented()){
                onGene5End = !onGene5End;
            }
            result += tr("%1'-end of gene. ").arg(onGene5End ? 5 : 3);
            if(complitelyOut){
                result += tr("Non-coding exon: %1").arg(Genbank::LocationParser::buildLocationString(QVector<U2Region>()<<exon));
            }else{
                result += tr("Non-coding part of %1'-coding exon: %2").arg(onGene5End ? 3 : 5).arg(Genbank::LocationParser::buildLocationString(QVector<U2Region>()<<exon));
            }
            //check if in splice site
            int spliceSiteLen = SPLICE_SITE_LEN;
            bool isDonor = false;
            bool isAcceptor = false;
            int exonsSize = exons.size();
            for(int i = 0; i < exons.size(); i++){
                const U2Region& reg = exons.at(i);
                if( ( (i!=0 || gene.isComplemented()) &&  (i != exonsSize - 1 || !gene.isComplemented()) )
                    && VariationPropertiesUtils::isInAcceptorSpliceSite(reg, variant.startPos, spliceSiteLen) ){
                    isAcceptor = true;
                    break;
                }
                if ((( (i!=0 || !gene.isComplemented()) &&  (i != exonsSize - 1 || gene.isComplemented()) )
                    &&VariationPropertiesUtils::isInDonorSpliceSite(reg, variant.startPos, spliceSiteLen)) ){
                    isDonor = true;
                    break;
                }
            }
            if (isDonor){
                if (gene.isComplemented()){
                    isDonor = !isDonor;
                }
                result += tr(". %1 splice-site.").arg(isDonor ? "Donor" : "Acceptor");
            }
            if (isAcceptor){
                if (gene.isComplemented()){
                    isAcceptor = !isAcceptor;
                }
                result += tr(". %1 splice-site.").arg(isAcceptor ? "Acceptor" : "Donor");
            }
        }

    } 

    return result;

}

QString VariationInfo::gentFullGeneReport( const QString& geneName ){
    QString res;
    if(!containsGene(geneName)){
        return res;
    }
    Gene gene = getGeneByName(geneName);
    bool isRNA = gene.getName().startsWith("FR");
    //name
    res += QString("ID: %1 ").arg(gene.getName());
    if(!gene.getAltName().isEmpty()){
        if (!isRNA){
            res += QString("(HUGO: %1) ").arg(gene.getAltName());
        }else{
            res += QString("(fRNAdb): %1) ").arg(gene.getAltName());
        }
        
    }
    if(!gene.getAccession().isEmpty()){
        res += QString("Accession (UniProt): %1 ").arg(gene.getAccession());
    }

    //region
    res += QString("Strand: %1 Region: %2")
        .arg(gene.isComplemented() ? "-" : "+")
        .arg(Genbank::LocationParser::buildLocationString(QVector<U2Region>()<<gene.getRegion()));

    if (!isRNA){
        res += QString(" CDS: %3").arg(QString("%1,%2").arg(gene.getCDSRegion().startPos+1).arg(gene.getCDSRegion().endPos()));
    }
    
    res += "\n";

    //exons
    res += QString("Exons: %1").arg(Genbank::LocationParser::buildLocationString(gene.getExons()));
    res += "\n";

    //description
    res += QString("Description: %1 ").arg(gene.getNote());
    res += "\n";

    //disease
    res += QString("Clinical Significance: %1 ").arg(gene.getDisease());
    res += "\n";

    res += QString("Gene type: %1 ").arg(gene.getType());
    res += "\n";

    //variation location
    res += QString("Variation location: %1 ").arg(getInGeneLocationInfo(geneName));
    res += "\n";

    //variation effect
    res += QString("Variation effect: %1 ").arg(getInGeneEffectInfo(geneName));
    res += "\n";

    return res;
}

bool VariationInfo::containsGene( const QString& name ) const{
    if(!genesFound){
        return false;
    }
    bool found = false;

    foreach(const Gene& gene, genes){
        if(gene.getName() == name){
            found = true;
            break;
        }
    }
    return found;
}

Gene VariationInfo::getGeneByName( const QString& name ) const{
    Gene found;

    foreach(const Gene& gene, genes){
        if(gene.getName() == name){
            found = gene;
            break;
        }
    }
    return found;
}

bool VariationInfo::isIntergenic(){
    if(!genesFound){
        return false;
    }
    if(genes.isEmpty()){
        return true;
    }
    return false;
}

QString VariationInfo::getNearGenes(const QList<Gene>& predefinedGenes){
    QString text;
    if (!isIntergenic()){
        return text;
    }
    text = "INTERGENIC.";
    /*QList<Gene> genesAround = predefinedGenes;
    
    if (genesAround.isEmpty()){
        U2OpStatusImpl opStatus;
        QList<int> excludeList;
        excludeList.append(S3TablesUtils::ExcludeSubfeatures);
        excludeList.append(S3TablesUtils::ExcludeCDS);
        genesAround = S3TablesUtils::findGenesAround(seqId, VARIATION_REGION(variant), AppContext::getSession()->getDatabase()->getDbi().dbi->getFeatureDbi(), opStatus, excludeList);
        CHECK_OP(opStatus, text);
    }
    if (genesAround.size() == 2){
        QString genename1;
        QString genename2;
        genename1 = genesAround.at(0).getName();
        genename2 = genesAround.at(1).getName();
        text.append(QString(" Between %1 and %2").arg(genename1).arg(genename2));
    }else if (genesAround.size() == 1){
        QString genename1;
        genename1 = genesAround.at(0).getName();
        text.append(QString(" Near %1").arg(genename1));
    }*/
    return text;
}

QString VariationInfo::getFullVariationReport(){
    QString result;
    result += "#VARIATION";
    result += "\n";
    result += getVariationInfoHeader();
    result += "\n";
    if(isIntergenic()){
        result += getNearGenes();
        result += "\n";
    } else{
        result += "#INTERSECTED GENES";
        result += "\n";
        foreach(const Gene& gene, genes){
            result += gentFullGeneReport(gene.getName());
        }
    }
    result += "\n";
    return result;    
}

QString VariationInfo::getVariationInfoHeader(){
    QString res;

    if(sequenceName.isEmpty()){
        U2OpStatusImpl opStatus;
        U2Sequence seq = seqDbi->getSequenceObject(seqId, opStatus);
        CHECK_OP(opStatus, res);
        sequenceName = seq.visualName;
    }
    
    res += QString("%1 %2 %3-->%4").arg(sequenceName).arg(variant.startPos + 1).arg(QString::fromAscii(variant.refData)).arg(QString::fromAscii(variant.obsData));

    return res;
}

QString VariationInfo::getInGeneTableHeader(){
    QString res;

    res = "#Chr\tPosition\tAllele\tdbSNP\tGene\tClinical_significance\tLocation\tProtein\tCodon\tSubstitution\tSIFTeffect\tSIFTscore";

    return res;

}

QStringList VariationInfo::getInGeneTableRaws(){
    QStringList res;

    if(!isIntergenic()){
        foreach(const Gene& gene, genes){
            QString curRaw = "";
            curRaw += gentFullGeneReport(gene.getName());
            if(sequenceName.isEmpty()){
                U2OpStatusImpl opStatus;
                SAFE_POINT(seqDbi!= NULL, "No sequence DBI", res);
                U2Sequence seq = seqDbi->getSequenceObject(seqId, opStatus);
                CHECK_OP(opStatus, res);
                sequenceName = seq.visualName;
            }
            curRaw += QString("%1\t%2\t%3/%4\t");
        }
    }

    
    return res;
}

void VariationInfo::initOrderColumns(){
    if (!columnsOrderInGene.isEmpty()){
        columnsOrderInGene.clear();
    }
    columnsOrderInGene << Chr << Position ; 
    /*    Allele,
        dbSNPId,
        GeneId,
        Clinical_significance,
        Location,
        Protein,
        Codon,
        SubstitutionAA,
        SIFTeffect,
        SIFTscore*/
}

void VariationInfo::addValueToRaw( const QString& val, ReportColumns key, QMap<ReportColumns, QString>& rawData ){
    if (columnsOrderInGene.contains(key)){
        rawData.insert(key, val);
    }
}

} //namespace
