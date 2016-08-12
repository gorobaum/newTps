#include "runinstance.h"

#include <iostream>

#include "tps/cudatps.h"
#include "tps/basictps.h"
#include "tps/paralleltps.h"
#include "linearsystem/armalinearsystems.h"
#include "linearsystem/cudalinearsystems.h"

namespace tps {

void RunInstance::loadData() {
    targetImage_ = loadImageData("targetImage");
    loadKeypoints();
}

void RunInstance::loadKeypoints() {
    if (instanceConfiguration_.isCallFeatureGenerator())
        generateKeypoints();
    else {
        targetKeypoints_ = instanceConfiguration_.getFloatVector("targetKeypoints");
        referenceKeypoints_ = instanceConfiguration_.getFloatVector("referenceKeypoints");
    }
    generateKeypointImage();
}

void RunInstance::generateKeypoints() {
    float percentage = instanceConfiguration_.getFloat("percentage");
    FeatureGenerator featureGenerator(referenceImage_, targetImage_, percentage);
    featureGenerator.run();

    targetKeypoints_ = featureGenerator.getTargetKeypoints();
    referenceKeypoints_ = featureGenerator.getReferenceKeypoints();
}

void RunInstance::solveLinearSystem() {
    std::string solverConfig = instanceConfiguration_.getString("linearSystemSolver");
    if (solverConfig.compare("cuda") == 0)
        solveLinearSystemWithCuda();
    else if (solverConfig.compare("armadillo") == 0) {
        solveLinearSystemWithArmadillo();
    } else {
        std::cout << "The solver \"" << solverConfig << "\" isn't present." << std::endl;
    }
}

void RunInstance::executeTps() {
    std::string tpsConfig = instanceConfiguration_.getString("tps");
    Image result(referenceImage_.getDimensions());

    if (tpsConfig.compare("basic") == 0)
        result = executeBasicTps();
    else if (tpsConfig.compare("parallel") == 0) {
        result = executeParallelTps();
    } else if (tpsConfig.compare("cuda") == 0) {
        result = executeCudaTps();
    } else {
        std::cout << "The TPS algorithm \"" << tpsConfig << "\" isn't present." << std::endl;
    }

    std::string resultImage = instanceConfiguration_.getString("resultImage");
    imageHandler_->saveImageData(result, resultImage);
}

Image RunInstance::executeBasicTps() {
    BasicTPS basicTps(referenceKeypoints_, targetKeypoints_, targetImage_);
    basicTps.setLinearSystemSolutions(solutionX_, solutionY_, solutionZ_);
    return basicTps.run();
}

Image RunInstance::executeParallelTps() {
    ParallelTPS parallelTps(referenceKeypoints_, targetKeypoints_, targetImage_);
    parallelTps.setLinearSystemSolutions(solutionX_, solutionY_, solutionZ_);
    return parallelTps.run();
}

Image RunInstance::executeCudaTps() {
    CudaTPS cudaTps(referenceKeypoints_, targetKeypoints_, targetImage_);
    cudaTps.setCudaMemory(cudaMemory_);
    return cudaTps.run();
}

void RunInstance::solveLinearSystemWithCuda() {
    bool isTwoDimensional = referenceImage_.isTwoDimensional();
    CudaLinearSystems cudaLinearSystems(referenceKeypoints_, targetKeypoints_,
                                        isTwoDimensional);

    std::vector<int> dimensions = referenceImage_.getDimensions();
    cudaMemory_.initialize(dimensions, referenceKeypoints_, targetImage_);

    cudaLinearSystems.solveLinearSystems(cudaMemory_);

    solutionX_ = cudaLinearSystems.getSolutionX();
    solutionY_ = cudaLinearSystems.getSolutionY();
    solutionZ_ = cudaLinearSystems.getSolutionZ();
}

void RunInstance::solveLinearSystemWithArmadillo() {
    bool isTwoDimensional = referenceImage_.isTwoDimensional();
    ArmaLinearSystems armaLinearSystems(referenceKeypoints_, targetKeypoints_,
                                        isTwoDimensional);

    armaLinearSystems.solveLinearSystems();

    solutionX_ = armaLinearSystems.getSolutionX();
    solutionY_ = armaLinearSystems.getSolutionY();
    solutionZ_ = armaLinearSystems.getSolutionZ();
}

void RunInstance::generateKeypointImage() {
    Image result(referenceImage_.getDimensions());

    for (int i = 0; i < targetKeypoints_.size(); i++)
        for (int j = 0; j < targetKeypoints_[i].size(); j++) {
            int x = std::ceil(targetKeypoints_[i][0]);
            int y = std::ceil(targetKeypoints_[i][1]);
            int z = std::ceil(targetKeypoints_[i][2]);
            result.changePixelAt(x, y, z, 255);
        }
    imageHandler_->saveImageData(result, "keypoints");
}

Image RunInstance::loadImageData(std::string configString) {
    std::string imagePath = instanceConfiguration_.getString(configString);
    return imageHandler_->loadImageData(imagePath);
}


} //namespace
