#include <QMessageBox>
#include <QtGui>
#include <QPushButton>
#include <QApplication>
#include <QtConcurrent>

#include <HDTManager.hpp>

#include "hdtoperation.hpp"

//#define OPERATION_CANCELABLE

HDTOperationDialog::HDTOperationDialog()
{
}

void HDTOperationDialog::closeEvent(QCloseEvent *event)
{
#ifdef OPERATION_CANCELABLE
    event->accept();
#else
    event->ignore();
#endif
}

HDTOperation::HDTOperation() : hdt(NULL), hdtInfo(NULL), errorMessage(NULL)
{
}

HDTOperation::HDTOperation(QString fileName) : fileName(fileName), hdt(NULL), hdtInfo(NULL), errorMessage(NULL)
{
}

HDTOperation::HDTOperation(hdt::HDT *hdt) : hdt(hdt), hdtInfo(NULL), errorMessage(NULL)
{
}

HDTOperation::HDTOperation(hdt::HDT *hdt, HDTCachedInfo *hdtInfo) : hdt(hdt), hdtInfo(hdtInfo), errorMessage(NULL)
{
}

void HDTOperation::execute() {
    try {
        switch(op) {
        case HDT_READ: {
            hdt::IntermediateListener iListener(dynamic_cast<ProgressListener *>(this));
            iListener.setRange(0,70);
#if 1
            hdt = hdt::HDTManager::mapIndexedHDT(fileName.toLatin1(), &iListener);
#else            
            hdt = hdt::HDTManager::loadIndexedHDT(fileName.toLatin1(), &iListener);
#endif
            iListener.setRange(70, 90);

            iListener.setRange(90, 100);
            hdtInfo = new HDTCachedInfo(hdt);
            if(fileName.endsWith(".gz")){
                fileName.left(fileName.length()-3);
            }
            QString infoFile = fileName + ".hdtcache";
            hdtInfo->load(infoFile, &iListener);

            break;
        }
        case RDF_READ:{
            hdt::IntermediateListener iListener(dynamic_cast<ProgressListener *>(this));

            iListener.setRange(0,80);
            hdt = hdt::HDTManager::generateHDT(fileName.toLatin1(), baseUri.c_str(), notation, spec, &iListener);

            iListener.setRange(80, 90);
            hdt = hdt::HDTManager::indexedHDT(hdt);

            iListener.setRange(90, 100);
            hdtInfo = new HDTCachedInfo(hdt);
            hdtInfo->generateGeneralInfo(&iListener);
            hdtInfo->generateMatrix(&iListener);

            break;
            }
        case HDT_WRITE:
            hdt->saveToHDT(fileName.toLatin1(), dynamic_cast<ProgressListener *>(this));
            break;
        case RDF_WRITE:{
            hdt::RDFSerializer *serializer = hdt::RDFSerializer::getSerializer(fileName.toLatin1(), notation);
            hdt->saveToRDF(*serializer, dynamic_cast<ProgressListener *>(this));
            delete serializer;
            break;
            }
        case RESULT_EXPORT: {
            hdt::RDFSerializer *serializer = hdt::RDFSerializer::getSerializer(fileName.toLatin1(), notation);
            serializer->serialize(iterator, dynamic_cast<ProgressListener *>(this), numResults );
            delete serializer;
            delete iterator;
            break;
        }
        }
        emit processFinished(0);
    } catch (char* err) {
        cout << "Error caught: " << err << endl;
        errorMessage = err;
        emit processFinished(1);
    } catch (const char* err) {
        cout << "Error caught: " << err << endl;
        errorMessage = (char *)err;
        emit processFinished(1);
    }
}


void HDTOperation::notifyProgress(float level, const char *section) {
#ifdef OPERATION_CANCELABLE
    if(isCancelled) {
        cout << "Throwing exception to cancel" << endl;
        throw (char *)"Cancelled by user";
    }
#endif
    emit progressChanged((int)level);
    emit messageChanged(QString(section));
}

void HDTOperation::saveToRDF(QString &fileName, hdt::RDFNotation notation)
{
    this->op = RDF_WRITE;
    this->fileName = fileName;
    this->notation = notation;
}

void HDTOperation::saveToHDT(QString &fileName)
{
    this->op = HDT_WRITE;
    this->fileName = fileName;
}

void HDTOperation::loadFromRDF(hdt::HDTSpecification &spec, QString &fileName, hdt::RDFNotation notation, string &baseUri)
{
    this->op = RDF_READ;
    this->spec = spec;
    this->fileName = fileName;
    this->notation = notation;
    this->baseUri = baseUri;
}

void HDTOperation::loadFromHDT(QString &fileName)
{
    this->op = HDT_READ;
    this->fileName = fileName;
}

void HDTOperation::exportResults(QString &fileName, hdt::IteratorTripleString *iterator, unsigned int numResults, hdt::RDFNotation notation)
{
    this->op = RESULT_EXPORT;
    this->fileName = fileName;
    this->iterator = iterator;
    this->numResults = numResults;
    this->notation = notation;
}

hdt::HDT *HDTOperation::getHDT()
{
    return hdt;
}

HDTCachedInfo *HDTOperation::getHDTInfo()
{
    return hdtInfo;
}


int HDTOperation::exec()
{
    dialog.setRange(0,100);
    dialog.setAutoClose(false);
    dialog.setFixedSize(400,130);

    switch(op) {
    case HDT_READ:
        dialog.setWindowTitle(tr("Loading HDT File"));
        break;
    case RDF_READ:
        dialog.setWindowTitle(tr("Importing RDF File to HDT"));
        break;
    case HDT_WRITE:
        dialog.setWindowTitle(tr("Saving HDT File"));
        break;
    case RDF_WRITE:
        dialog.setWindowTitle(tr("Exporting HDT File to RDF"));
        break;
    case RESULT_EXPORT:
        dialog.setWindowTitle(tr("Exporting results to RDF"));
        break;
    }

#ifndef OPERATION_CANCELABLE
    QPushButton btn;
    btn.setEnabled(false);
    btn.setText(tr("Cancel"));
    dialog.setCancelButton(&btn);
#endif

    connect(this, SIGNAL(progressChanged(int)), &dialog, SLOT(setValue(int)), Qt::QueuedConnection);
    connect(this, SIGNAL(messageChanged(QString)), &dialog, SLOT(setLabelText(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(processFinished(int)), &dialog, SLOT(done(int)), Qt::QueuedConnection);
    connect(&dialog, SIGNAL(canceled()), this, SLOT(cancel()));

    isCancelled=false;
    QtConcurrent::run(this, &HDTOperation::execute);
    int result = dialog.exec();

    //cout << "Dialog returned" << endl;

    if(errorMessage) {
        result = 1;
        QMessageBox::critical(NULL, "ERROR", QString(errorMessage) );
    }

    QApplication::alert(QApplication::activeWindow());
    return result;
}

void HDTOperation::cancel()
{
    //cout << "Operation cancelled" << endl;
    isCancelled = true;
}



