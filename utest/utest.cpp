// kate: replace-tabs off; indent-width 4; indent-mode normal
// vim: ts=4:sw=4:noexpandtab
/*

Copyright (c) 2010--2011,
François Pomerleau and Stephane Magnenat, ASL, ETHZ, Switzerland
You can contact the authors at <f dot pomerleau at gmail dot com> and
<stephane at magnenat dot net>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ETH-ASL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "pointmatcher/PointMatcher.h"
#include "gtest/gtest.h"
#include <string>

using namespace std;
using namespace PointMatcherSupport;

// TODO: avoid global by using testing::Environment
typedef PointMatcher<float> PM;
std::string dataPath;

PM::DataPoints ref2D;
PM::DataPoints data2D;
PM::DataPoints ref3D;
PM::DataPoints data3D;
PM::TransformationParameters validT2d;
PM::TransformationParameters validT3d;


class IcpHelper: public testing::Test
{
public:
	
	PM pm;
	PM::ICP icp;
	
	PM::Parameters params;

	virtual void dumpVTK()
	{
		// Make available a VTK inspector for manual inspection
		icp.inspector.reset(pm.InspectorRegistrar.create(
			"VTKFileInspector", 
			PM::Parameters({{"baseFileName","./unitTest"}})
			)
		);
	}
	
	void validate2dTransformation()
	{
		const PM::TransformationParameters testT = icp(data2D, ref2D);
		const int dim = validT2d.cols();

		const auto validTrans = validT2d.block(0, dim-1, dim-1, 1).norm();
		const auto testTrans = testT.block(0, dim-1, dim-1, 1).norm();
	
		const auto validAngle = acos(validT2d(0,0));
		const auto testAngle = acos(testT(0,0));
		
		EXPECT_NEAR(validTrans, testTrans, 0.05);
		EXPECT_NEAR(validAngle, testAngle, 0.05);
	}

	void validate3dTransformation()
	{
		//dumpVTK();

		const PM::TransformationParameters testT = icp(data3D, ref3D);
		const int dim = validT2d.cols();

		const auto validTrans = validT3d.block(0, dim-1, dim-1, 1).norm();
		const auto testTrans = testT.block(0, dim-1, dim-1, 1).norm();
	
		const auto testRotation = Eigen::Quaternion<float>(Eigen::Matrix<float,3,3>(testT.topLeftCorner(3,3)));
		const auto validRotation = Eigen::Quaternion<float>(Eigen::Matrix<float,3,3>(validT3d.topLeftCorner(3,3)));
		
		const auto angleDist = validRotation.angularDistance(testRotation);
		
		//cout << testT << endl;
		//cout << "angleDist: " << angleDist << endl;
		//cout << "transDist: " << abs(validTrans-testTrans) << endl;
		EXPECT_NEAR(validTrans, testTrans, 0.1);
		EXPECT_NEAR(angleDist, 0.0, 0.1);

	}
};

// Utility classes
class GenericTest: public IcpHelper
{

public:

	// Will be called for every tests
	virtual void SetUp()
	{
		icp.setDefault();
		// Uncomment for consol outputs
		//setLogger(pm.LoggerRegistrar.create("FileLogger"));
	}

	// Will be called for every tests
	virtual void TearDown()
	{	
	}
};


//---------------------------
// Generic tests
//---------------------------

TEST_F(GenericTest, ICP_default)
{
	validate2dTransformation();
	validate3dTransformation();
}

//---------------------------
// DataFilter modules
//---------------------------

// Utility classes
class DataFilterTest: public IcpHelper
{

public:

	PM::DataPointsFilter* testedDataPointFilter;

	// Will be called for every tests
	virtual void SetUp()
	{
		icp.setDefault();
		// Uncomment for consol outputs
		//setLogger(pm.LoggerRegistrar.create("FileLogger"));
		
		// We'll test the filters on reading point cloud
		icp.readingDataPointsFilters.clear();
	}

	// Will be called for every tests
	virtual void TearDown()	{}

	void addFilter(string name, PM::Parameters params)
	{
		testedDataPointFilter = 
			pm.DataPointsFilterRegistrar.create(name, params);
	
		icp.readingDataPointsFilters.push_back(testedDataPointFilter);
	}
	
	void addFilter(string name)
	{
		testedDataPointFilter = 
			pm.DataPointsFilterRegistrar.create(name);
		
		icp.readingDataPointsFilters.push_back(testedDataPointFilter);
	}
};

TEST_F(DataFilterTest, MaxDistDataPointsFilter)
{
	// Max dist has been selected to not affect the points
	params = PM::Parameters({{"dim","0"}, {"maxDist", toParam(6.0)}});
	
	// Filter on x axis
	params["dim"] = "0";
	icp.readingDataPointsFilters.clear();
	addFilter("MaxDistDataPointsFilter", params);
	validate2dTransformation();
	validate3dTransformation();
	
	// Filter on y axis
	params["dim"] = "1";
	icp.readingDataPointsFilters.clear();
	addFilter("MaxDistDataPointsFilter", params);
	validate2dTransformation();	
	validate3dTransformation();
	
	// Filter on z axis (not existing)
	params["dim"] = "2";
	icp.readingDataPointsFilters.clear();
	addFilter("MaxDistDataPointsFilter", params);
	EXPECT_ANY_THROW(validate2dTransformation());
	validate3dTransformation();
	
	// Filter on a radius
	params["dim"] = "-1";
	icp.readingDataPointsFilters.clear();
	addFilter("MaxDistDataPointsFilter", params);
	validate2dTransformation();	
	validate3dTransformation();
	
	// Parameter outside valid range
	params["dim"] = "3";
	//TODO: specify the exception, move that to GenericTest
	EXPECT_ANY_THROW(addFilter("MaxDistDataPointsFilter", params));
	
}


TEST_F(DataFilterTest, MinDistDataPointsFilter)
{
	// Min dist has been selected to not affect the points too much
	params = PM::Parameters({{"dim","0"}, {"minDist", toParam(0.05)}});
	
	// Filter on x axis
	params["dim"] = "0";
	icp.readingDataPointsFilters.clear();
	addFilter("MinDistDataPointsFilter", params);
	validate2dTransformation();	
	validate3dTransformation();
	
	// Filter on y axis
	params["dim"] = "1";
	icp.readingDataPointsFilters.clear();
	addFilter("MinDistDataPointsFilter", params);
	validate2dTransformation();	
	validate3dTransformation();
	
	//TODO: move that to specific 2D test
	// Filter on z axis (not existing)
	params["dim"] = "2";
	icp.readingDataPointsFilters.clear();
	addFilter("MinDistDataPointsFilter", params);
	EXPECT_ANY_THROW(validate2dTransformation());
	validate3dTransformation();
	
	// Filter on a radius
	params["dim"] = "-1";
	icp.readingDataPointsFilters.clear();
	addFilter("MinDistDataPointsFilter", params);
	validate2dTransformation();	
	validate3dTransformation();
		
}

TEST_F(DataFilterTest, MaxQuantileOnAxisDataPointsFilter)
{
	// Ratio has been selected to not affect the points too much
	string ratio = "0.95";
	params = PM::Parameters({{"dim","0"}, {"ratio", ratio}});
	
	// Filter on x axis
	params["dim"] = "0";
	icp.readingDataPointsFilters.clear();
	addFilter("MaxQuantileOnAxisDataPointsFilter", params);
	validate2dTransformation();	
	validate3dTransformation();
	
	// Filter on y axis
	params["dim"] = "1";
	icp.readingDataPointsFilters.clear();
	addFilter("MaxQuantileOnAxisDataPointsFilter", params);
	validate2dTransformation();	
	validate3dTransformation();
	
	// Filter on z axis (not existing)
	params["dim"] = "2";
	icp.readingDataPointsFilters.clear();
	addFilter("MaxQuantileOnAxisDataPointsFilter", params);
	EXPECT_ANY_THROW(validate2dTransformation());	
	validate3dTransformation();
}

// TODO; replace by MaxDensityDataPointsFilter
// TEST_F(DataFilterTest, UniformizeDensityDataPointsFilter)
// {
// 	// Ratio has been selected to not affect the points too much
// 	vector<double> ratio = vector<double>({0.1, 0.15});
// 
// 	for(unsigned i=0; i < ratio.size(); i++)
// 	{
// 		params = PM::Parameters({{"ratio", toParam(ratio[i])}, {"nbBin", "20"}});
// 		icp.readingDataPointsFilters.clear();
// 		addFilter("UniformizeDensityDataPointsFilter", params);
// 		validate2dTransformation();	
// 
// 		const double nbInitPts = data2D.features.cols();
// 		const double nbRemainingPts = icp.getPrefilteredReadingPtsCount();
// 		// FIXME: 10% seems of seems a little bit high
// 		EXPECT_NEAR(nbRemainingPts/nbInitPts, 1-ratio[i], 0.10);
// 		
// 		validate3dTransformation();
// 		//TODO: add expectation on the reduction
// 	}
// }

TEST_F(DataFilterTest, SurfaceNormalDataPointsFilter)
{
	// This filter create descriptor, so parameters should'nt impact results
	params = PM::Parameters({
		{"knn", "5"}, 
		{"epsilon", "0.1"}, 
		{"keepNormals", "1"},
		{"keepDensities", "1"},
		{"keepEigenValues", "1"},
		{"keepEigenVectors", "1" },
		{"keepMatchedIds" , "1" }
		});
	// FIXME: the parameter keepMatchedIds seems to do nothing...

	addFilter("SurfaceNormalDataPointsFilter", params);
	validate2dTransformation();	
	validate3dTransformation();

}

TEST_F(DataFilterTest, SamplingSurfaceNormalDataPointsFilter)
{
	// This filter create descriptor AND subsample
	params = PM::Parameters({
		{"binSize", "5"}, 
		{"averageExistingDescriptors", "1"}, 
		{"keepNormals", "1"},
		{"keepDensities", "1"},
		{"keepEigenValues", "1"},
		{"keepEigenVectors", "1" },
		{"keepMatchedIds" , "1" }
		});
	
	addFilter("SamplingSurfaceNormalDataPointsFilter", params);
	validate2dTransformation();	
	validate3dTransformation();

}

TEST_F(DataFilterTest, OrientNormalsDataPointsFilter)
{
	// Used to create normal for reading point cloud
	PM::DataPointsFilter* extraDataPointFilter;
	extraDataPointFilter = pm.DataPointsFilterRegistrar.create(
			"SurfaceNormalDataPointsFilter");
	icp.readingDataPointsFilters.push_back(extraDataPointFilter);
	addFilter("ObservationDirectionDataPointsFilter");
	params = PM::Parameters({{"towardCenter", toParam(false)}});
	addFilter("OrientNormalsDataPointsFilter", params);
	validate2dTransformation();	
	validate3dTransformation();

}


TEST_F(DataFilterTest, RandomSamplingDataPointsFilter)
{
	vector<double> prob = {0.80, 0.85, 0.90, 0.95};
	for(unsigned i=0; i<prob.size(); i++)
	{
		// Try to avoid to low value for the reduction to avoid under sampling
		params = PM::Parameters({{"prob", toParam(prob[i])}});
		icp.readingDataPointsFilters.clear();
		addFilter("RandomSamplingDataPointsFilter", params);
		validate2dTransformation();	
		validate3dTransformation();
	}
}


TEST_F(DataFilterTest, FixStepSamplingDataPointsFilter)
{
	vector<unsigned> steps = {1, 2, 3};
	for(unsigned i=0; i<steps.size(); i++)
	{
		// Try to avoid too low value for the reduction to avoid under sampling
		params = PM::Parameters({{"startStep", toParam(steps[i])}});
		icp.readingDataPointsFilters.clear();
		addFilter("FixStepSamplingDataPointsFilter", params);
		validate2dTransformation();	
		validate3dTransformation();
	}
}

//---------------------------
// Matcher modules
//---------------------------

// Utility classes
class MatcherTest: public IcpHelper
{

public:

	PM::Matcher* testedMatcher;

	// Will be called for every tests
	virtual void SetUp()
	{
		icp.setDefault();
		// Uncomment for consol outputs
		//setLogger(pm.LoggerRegistrar.create("FileLogger"));
	}

	// Will be called for every tests
	virtual void TearDown(){}

	void addFilter(string name, PM::Parameters params)
	{
		testedMatcher = 
			pm.MatcherRegistrar.create(name, params);
		icp.matcher.reset(testedMatcher);
	}

};

TEST_F(MatcherTest, KDTreeMatcher)
{
	vector<unsigned> knn = {1, 2, 3};
	vector<double> epsilon = {0.0, 0.2};
	vector<double> maxDist = {1.0, 0.5};

	for(unsigned i=0; i < knn.size(); i++)
	{
		for(unsigned j=0; j < epsilon.size(); j++)
		{
			for(unsigned k=0; k < maxDist.size(); k++)
			{
				params = PM::Parameters({
					{"knn", toParam(knn[i])}, // remove end parenthesis for bug
					{"epsilon", toParam(epsilon[j])}, 
					{"searchType", "1"},
					{"maxDist", toParam(maxDist[k])},
					});
			
				addFilter("KDTreeMatcher", params);
				validate2dTransformation();
				validate3dTransformation();
			}
		}
	}
}


//---------------------------
// Outlier modules
//---------------------------

// Utility classes
class OutlierFilterTest: public IcpHelper
{
public:
	PM::OutlierFilter* testedOutlierFilter;

	// Will be called for every tests
	virtual void SetUp()
	{
		icp.setDefault();
		// Uncomment for consol outputs
		//setLogger(pm.LoggerRegistrar.create("FileLogger"));
		
		icp.outlierFilters.clear();
	}

	// Will be called for every tests
	virtual void TearDown(){}

	void addFilter(string name, PM::Parameters params)
	{
		testedOutlierFilter = 
			pm.OutlierFilterRegistrar.create(name, params);
		icp.outlierFilters.push_back(testedOutlierFilter);
	}

};


//No commun parameters were found for 2D and 3D, tests are splited
TEST_F(OutlierFilterTest, MaxDistOutlierFilter2D)
{
	params = PM::Parameters({{"maxDist", toParam(0.015)}});//0.02
	addFilter("MaxDistOutlierFilter", params);
	validate2dTransformation();
}

TEST_F(OutlierFilterTest, MaxDistOutlierFilter3D)
{
	params = PM::Parameters({{"maxDist", toParam(0.1)}});
	addFilter("MaxDistOutlierFilter", params);
	validate3dTransformation();
}

//No commun parameters were found for 2D and 3D, tests are splited
TEST_F(OutlierFilterTest, MinDistOutlierFilter2D)
{
	// Since not sure how useful is that filter, we keep the 
	// MaxDistOutlierFilter with it
	PM::OutlierFilter* extraOutlierFilter;
	
	params = PM::Parameters({{"maxDist", toParam(0.015)}});
	extraOutlierFilter = 
			pm.OutlierFilterRegistrar.create("MaxDistOutlierFilter", params);
	icp.outlierFilters.push_back(extraOutlierFilter);	
	
	params = PM::Parameters({{"minDist", toParam(0.0002)}});
	addFilter("MinDistOutlierFilter", params);
	
	validate2dTransformation();
}

TEST_F(OutlierFilterTest, MinDistOutlierFilter3D)
{
	// Since not sure how useful is that filter, we keep the 
	// MaxDistOutlierFilter with it
	PM::OutlierFilter* extraOutlierFilter;
	
	params = PM::Parameters({{"maxDist", toParam(0.1)}});
	extraOutlierFilter = 
			pm.OutlierFilterRegistrar.create("MaxDistOutlierFilter", params);
	icp.outlierFilters.push_back(extraOutlierFilter);	
	
	params = PM::Parameters({{"minDist", toParam(0.0002)}});
	addFilter("MinDistOutlierFilter", params);
	
	validate3dTransformation();
}

TEST_F(OutlierFilterTest, MedianDistOutlierFilter)
{
	params = PM::Parameters({{"factor", toParam(3.5)}});
	addFilter("MedianDistOutlierFilter", params);
	validate2dTransformation();
	validate3dTransformation();
}


TEST_F(OutlierFilterTest, TrimmedDistOutlierFilter)
{
	params = PM::Parameters({{"ratio", toParam(0.85)}});
	addFilter("TrimmedDistOutlierFilter", params);
	validate2dTransformation();
	validate3dTransformation();
}


TEST_F(OutlierFilterTest, VarTrimmedDistOutlierFilter)
{
	params = PM::Parameters({
		{"minRatio", toParam(0.60)},
		{"maxRatio", toParam(0.80)},
		{"lambda", toParam(0.9)},
	});
	addFilter("VarTrimmedDistOutlierFilter", params);
	validate2dTransformation();
	validate3dTransformation();
}

//---------------------------
// Error modules
//---------------------------

// Utility classes
class ErrorMinimizerTest: public IcpHelper
{
public:
	PM::ErrorMinimizer* errorMin;

	// Will be called for every tests
	virtual void SetUp()
	{
		icp.setDefault();
		// Uncomment for consol outputs
		//setLogger(pm.LoggerRegistrar.create("FileLogger"));
	}

	// Will be called for every tests
	virtual void TearDown(){}

	void addFilter(string name)
	{
		errorMin = pm.ErrorMinimizerRegistrar.create(name);
		icp.errorMinimizer.reset(errorMin);
	}
};


TEST_F(ErrorMinimizerTest, PointToPointErrorMinimizer)
{
	addFilter("PointToPointErrorMinimizer");	
	validate2dTransformation();
	validate3dTransformation();
}

TEST_F(ErrorMinimizerTest, PointToPlaneErrorMinimizer)
{
	addFilter("PointToPlaneErrorMinimizer");	
	validate2dTransformation();
	validate3dTransformation();
}

//---------------------------
// Transformation Checker modules
//---------------------------

// Utility classes
class TransformationCheckerTest: public IcpHelper
{
public:
	PM::TransformationChecker* transformCheck;

	// Will be called for every tests
	virtual void SetUp()
	{
		icp.setDefault();
		// Uncomment for consol outputs
		//setLogger(pm.LoggerRegistrar.create("FileLogger"));
		
		icp.transformationCheckers.clear();
	}

	// Will be called for every tests
	virtual void TearDown(){}

	void addFilter(string name, PM::Parameters params)
	{
		transformCheck = 
			pm.TransformationCheckerRegistrar.create(name, params);
		
		icp.transformationCheckers.push_back(transformCheck);
	}
};


TEST_F(TransformationCheckerTest, CounterTransformationChecker)
{
	params = PM::Parameters({{"maxIterationCount", toParam(20)}});
	addFilter("CounterTransformationChecker", params);
	validate2dTransformation();
}

TEST_F(TransformationCheckerTest, DifferentialTransformationChecker)
{
	params = PM::Parameters({
		{"minDiffRotErr", toParam(0.001)},
		{"minDiffTransErr", toParam(0.001)},
		{"smoothLength", toParam(4)}
	});
	
	addFilter("DifferentialTransformationChecker", params);
	validate2dTransformation();
}

TEST_F(TransformationCheckerTest, BoundTransformationChecker)
{
	// Since that transChecker is trigger when the distance is growing
	// and that we do not expect that to happen in the test dataset, we
	// keep the Counter to get out of the looop	
	PM::TransformationChecker* extraTransformCheck;
	
	params = PM::Parameters({
		{"maxRotationNorm", toParam(1.0)},
		{"maxTranslationNorm", toParam(1.0)}
	});
	
	extraTransformCheck = pm.TransformationCheckerRegistrar.create(
		"CounterTransformationChecker");
	icp.transformationCheckers.push_back(extraTransformCheck);
	
	addFilter("BoundTransformationChecker", params);
	validate2dTransformation();
}

//---------------------------
// Main
//---------------------------
int main(int argc, char **argv)
{
	dataPath = "";
	for(int i=1; i < argc; i++)
	{
		if(strcmp(argv[i], "--path") == 0 && i+1 < argc)
				dataPath = argv[i+1];
	}

	if(dataPath == "")
	{
		cerr << "Missing the flag --path ./path/to/examples/data\n Please give the path to the test data folder which should be included with the source code. The folder is named 'examples/data'." << endl;
		return -1;
	}

	// Load point cloud for all test
	ref2D =  PM::loadCSV(dataPath + "2D_oneBox.csv");
	data2D = PM::loadCSV(dataPath + "2D_twoBoxes.csv");
	ref3D =  PM::loadCSV(dataPath + "car_cloud400.csv");
	data3D = PM::loadCSV(dataPath + "car_cloud401.csv");
	

	// Result of data express in ref (from visual inspection)
	validT2d = PM::TransformationParameters(3,3);
	validT2d <<  0.987498,  0.157629, 0.0859918,
				-0.157629,  0.987498,  0.203247,
						0,         0,         1;

	validT3d = PM::TransformationParameters(4,4);
	validT3d <<   0.982304,   0.166685,  -0.0854066,  0.0446816,
	 			 -0.150189,   0.973488,   0.172524,   0.191998,
	   			  0.111899,  -0.156644,   0.981296,  -0.0356313,
	              0,          0,          0,          1;

	testing::GTEST_FLAG(print_time) = true;
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}



