#include "ARA.h"
const ARA::ARAFactory *JUCE_CALLTYPE createARAFactory() {
  return juce::ARADocumentControllerSpecialisation::createARAFactory<
      one::Controller>();
}
