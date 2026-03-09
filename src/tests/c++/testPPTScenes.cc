#include <gtest/gtest.h>
#include "../../DsscPpt/DsscPptScenes.hh"

TEST(DsscPptScenesTest, GetControlSceneForSensorType) {
    std::string scene = getControlScene("DETLAB", "Q1", false);
    EXPECT_EQ(scene.find("DEPFET"), std::string::npos)
        << "Expected NO DEPFET references in scene when isDEPFET false";

    scene = getControlScene("DETLAB", "Q1", true);
    EXPECT_NE(scene.find("DEPFET"), std::string::npos)
        << "Expected DEPFET references in scene when isDEPFET true";
}
