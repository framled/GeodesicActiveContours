/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkGeodesicActiveContourLevelSetImageFilter.h"
#include "itkCurvatureAnisotropicDiffusionImageFilter.h"
#include "itkGradientMagnitudeRecursiveGaussianImageFilter.h"
#include "itkSigmoidImageFilter.h"
#include "itkFastMarchingImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"

int main( int argc, char* argv[] )
{
	if( argc != 11 )
	{
		std::cerr << "Usage: " << argv[0];
		std::cerr << " <InputFileName>  <OutputFileName>" << std::endl;
		std::cerr << " <seedX> <seedY> <InitialDistance>" << std::endl;
		std::cerr << " <Sigma> <SigmoidAlpha> <SigmoidBeta>" << std::endl;
		std::cerr << " <PropagationScaling> <NumberOfIterations>"  << std::endl;
		return EXIT_FAILURE;
	}

  const char * inputFileName =      argv[1];
  const char * outputFileName =     argv[2];
  const int seedPosX =              atoi( argv[3] );
  const int seedPosY =              atoi( argv[4] );

  const double initialDistance =    atof( argv[5] );
  const double sigma =              atof( argv[6] );
  const double alpha =              atof( argv[7] );
  const double beta  =              atof( argv[8] );
  const double propagationScaling = atof( argv[9] );
  const double numberOfIterations = atoi( argv[10] );
  const double seedValue =          - initialDistance;

  const unsigned int                Dimension = 2;

  typedef float                                    InputPixelType;
  typedef itk::Image< InputPixelType, Dimension >  InputImageType;
  typedef unsigned char                            OutputPixelType;
  typedef itk::Image< OutputPixelType, Dimension > OutputImageType;

  typedef  itk::ImageFileReader< InputImageType >  ReaderType;
  typedef  itk::ImageFileWriter< OutputImageType > WriterType;

  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( inputFileName );

  typedef  itk::CurvatureAnisotropicDiffusionImageFilter< InputImageType, InputImageType > SmoothingFilterType;
  SmoothingFilterType::Pointer smoothing = SmoothingFilterType::New();
  smoothing->SetTimeStep( 0.125 );
  smoothing->SetNumberOfIterations( 5 );
  smoothing->SetConductanceParameter( 9.0 );
  smoothing->SetInput( reader->GetOutput() );

  typedef  itk::GradientMagnitudeRecursiveGaussianImageFilter< InputImageType, InputImageType > GradientFilterType;
  GradientFilterType::Pointer  gradientMagnitude = GradientFilterType::New();
  gradientMagnitude->SetSigma( sigma );
  gradientMagnitude->SetInput( smoothing->GetOutput() );

  typedef  itk::SigmoidImageFilter< InputImageType, InputImageType > SigmoidFilterType;
  SigmoidFilterType::Pointer sigmoid = SigmoidFilterType::New();
  sigmoid->SetOutputMinimum( 0.0 );
  sigmoid->SetOutputMaximum( 1.0 );
  sigmoid->SetAlpha( alpha );
  sigmoid->SetBeta( beta );
  sigmoid->SetInput( gradientMagnitude->GetOutput() );

  typedef  itk::FastMarchingImageFilter< InputImageType, InputImageType > FastMarchingFilterType;
  FastMarchingFilterType::Pointer  fastMarching = FastMarchingFilterType::New();

  typedef  itk::GeodesicActiveContourLevelSetImageFilter< InputImageType, InputImageType >  GeodesicActiveContourFilterType;
  GeodesicActiveContourFilterType::Pointer geodesicActiveContour = GeodesicActiveContourFilterType::New();
  geodesicActiveContour->SetPropagationScaling( propagationScaling );
  geodesicActiveContour->SetCurvatureScaling( 1.0 );
  geodesicActiveContour->SetAdvectionScaling( 1.0 );
  geodesicActiveContour->SetMaximumRMSError( 0.02 );
  geodesicActiveContour->SetNumberOfIterations( numberOfIterations );
  geodesicActiveContour->SetInput( fastMarching->GetOutput() );
  geodesicActiveContour->SetFeatureImage( sigmoid->GetOutput() );

  typedef itk::BinaryThresholdImageFilter< InputImageType, OutputImageType > ThresholdingFilterType;
  ThresholdingFilterType::Pointer thresholder = ThresholdingFilterType::New();
  thresholder->SetLowerThreshold( -1000.0 );
  thresholder->SetUpperThreshold( 0.0 );
  thresholder->SetOutsideValue( itk::NumericTraits< OutputPixelType >::min() );
  thresholder->SetInsideValue( itk::NumericTraits< OutputPixelType >::max() );
  thresholder->SetInput( geodesicActiveContour->GetOutput() );

  typedef FastMarchingFilterType::NodeContainer  NodeContainer;
  typedef FastMarchingFilterType::NodeType       NodeType;

  InputImageType::IndexType  seedPosition;
  seedPosition[0] = seedPosX;
  seedPosition[1] = seedPosY;

  NodeContainer::Pointer seeds = NodeContainer::New();
  NodeType node;
  node.SetValue( seedValue );
  node.SetIndex( seedPosition );

  seeds->Initialize();
  seeds->InsertElement( 0, node );

  fastMarching->SetTrialPoints( seeds );
  fastMarching->SetSpeedConstant( 1.0 );

  typedef itk::RescaleIntensityImageFilter< InputImageType, OutputImageType > CastFilterType;

  CastFilterType::Pointer caster1 = CastFilterType::New();
  CastFilterType::Pointer caster2 = CastFilterType::New();
  CastFilterType::Pointer caster3 = CastFilterType::New();
  CastFilterType::Pointer caster4 = CastFilterType::New();

  WriterType::Pointer writer1 = WriterType::New();
  WriterType::Pointer writer2 = WriterType::New();
  WriterType::Pointer writer3 = WriterType::New();
  WriterType::Pointer writer4 = WriterType::New();

  caster1->SetInput( smoothing->GetOutput() );
  writer1->SetInput( caster1->GetOutput() );
  writer1->SetFileName("GeodesicActiveContourImageFilterOutput1.png");
  caster1->SetOutputMinimum( itk::NumericTraits< OutputPixelType >::min() );
  caster1->SetOutputMaximum( itk::NumericTraits< OutputPixelType >::max() );
  writer1->Update();

  caster2->SetInput( gradientMagnitude->GetOutput() );
  writer2->SetInput( caster2->GetOutput() );
  writer2->SetFileName("GeodesicActiveContourImageFilterOutput2.png");
  caster2->SetOutputMinimum( itk::NumericTraits< OutputPixelType >::min() );
  caster2->SetOutputMaximum( itk::NumericTraits< OutputPixelType >::max() );
  writer2->Update();

  caster3->SetInput( sigmoid->GetOutput() );
  writer3->SetInput( caster3->GetOutput() );
  writer3->SetFileName("GeodesicActiveContourImageFilterOutput3.png");
  caster3->SetOutputMinimum( itk::NumericTraits< OutputPixelType >::min() );
  caster3->SetOutputMaximum( itk::NumericTraits< OutputPixelType >::max() );
  writer3->Update();

  caster4->SetInput( fastMarching->GetOutput() );
  writer4->SetInput( caster4->GetOutput() );
  writer4->SetFileName("GeodesicActiveContourImageFilterOutput4.png");
  caster4->SetOutputMinimum( itk::NumericTraits< OutputPixelType >::min() );
  caster4->SetOutputMaximum( itk::NumericTraits< OutputPixelType >::max() );

  fastMarching->SetOutputSize(
           reader->GetOutput()->GetBufferedRegion().GetSize() );

  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( outputFileName );
  writer->SetInput( thresholder->GetOutput() );
  try
    {
    writer->Update();
    }
  catch( itk::ExceptionObject & error )
    {
    std::cerr << "Error: " << error << std::endl;
    return EXIT_FAILURE;
    }

  std::cout << std::endl;
  std::cout << "Max. no. iterations: " << geodesicActiveContour->GetNumberOfIterations() << std::endl;
  std::cout << "Max. RMS error: " << geodesicActiveContour->GetMaximumRMSError() << std::endl;
  std::cout << std::endl;
  std::cout << "No. elpased iterations: " << geodesicActiveContour->GetElapsedIterations() << std::endl;
  std::cout << "RMS change: " << geodesicActiveContour->GetRMSChange() << std::endl;

  try
    {
    writer4->Update();
    }
  catch( itk::ExceptionObject & error )
    {
    std::cerr << "Error: " << error << std::endl;
    return EXIT_FAILURE;
    }

  typedef itk::ImageFileWriter< InputImageType > InternalWriterType;

  InternalWriterType::Pointer mapWriter = InternalWriterType::New();
  mapWriter->SetInput( fastMarching->GetOutput() );
  mapWriter->SetFileName("GeodesicActiveContourImageFilterOutput4.mha");
  try
    {
    mapWriter->Update();
    }
  catch( itk::ExceptionObject & error )
    {
    std::cerr << "Error: " << error << std::endl;
    return EXIT_FAILURE;
    }

  InternalWriterType::Pointer speedWriter = InternalWriterType::New();
  speedWriter->SetInput( sigmoid->GetOutput() );
  speedWriter->SetFileName("GeodesicActiveContourImageFilterOutput3.mha");
  try
    {
    speedWriter->Update();
    }
  catch( itk::ExceptionObject & error )
    {
    std::cerr << "Error: " << error << std::endl;
    return EXIT_FAILURE;
    }

  InternalWriterType::Pointer gradientWriter = InternalWriterType::New();
  gradientWriter->SetInput( gradientMagnitude->GetOutput() );
  gradientWriter->SetFileName("GeodesicActiveContourImageFilterOutput2.mha");
  try
    {
    gradientWriter->Update();
    }
  catch( itk::ExceptionObject & error )
    {
    std::cerr << "Error: " << error << std::endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
