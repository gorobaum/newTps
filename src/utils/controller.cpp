#include "controller.h"
#include "globalconfiguration.h"

namespace tps {

void Controller::exec() {
    for (std::vector<RunInstance>::iterator it = runInstances_.begin(); it != runInstances_.end(); it++)
        it->loadData();

    for (std::vector<RunInstance>::iterator it = runInstances_.begin(); it != runInstances_.end(); it++)
        it->solveLinearSystem();

    for (std::vector<RunInstance>::iterator it = runInstances_.begin(); it != runInstances_.end(); it++)
        it->executeTps();
}

void Controller::readConfigurationFile(std::string masterConfigFilePath) {
    GlobalConfiguration::getInstance().loadConfigurationFile(masterConfigFilePath);

    std::string referenceImagePath = GlobalConfiguration::getInstance().getString("referenceImage");

    // Based on the reference image name, load the correct ImageHandler and
    // load the reference image data to the memory
    ImageHandler* imageHandler = getCorrectImageHandler(referenceImagePath);
    Image referenceImage = imageHandler->loadImageData(referenceImagePath);

    std::vector<std::string> targetFiles = GlobalConfiguration::getInstance().getTargetFiles();

    for (std::vector<std::string>::iterator it = targetFiles.begin(); it != targetFiles.end(); it++) {
        RunInstance newRunInstance(*it, referenceImage, imageHandler);
        runInstances_.push_back(newRunInstance);
    }
}

ImageHandler* Controller::getCorrectImageHandler(std::string referenceImagePath) {
    std::size_t pos = referenceImagePath.find('.');
    std::string extension = referenceImagePath.substr(pos);

    tps::ImageHandler *imageHandler;

    if (extension.compare(".nii.gz") == 0)
        imageHandler = new tps::ITKImageHandler();
    else
        imageHandler = new tps::OPCVImageHandler();

    return imageHandler;
}


} //namespace
