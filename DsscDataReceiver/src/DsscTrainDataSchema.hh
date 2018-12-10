/* 
 * File:   DsscTrainDataSchema.h
 * Author: kirchgessner
 *
 * Created on April 28, 2016, 1:48 PM
 */

#ifndef DSSCTRAINDATASCHEMA_H
#define	DSSCTRAINDATASCHEMA_H

#define DSSC_TRAINDATA_SCHEMA_SIMPLE(schema)                                    \
                                                                                \
    NODE_ELEMENT(schema).key("INSTRUMENT")                                      \
                .commit();                                                      \
                                                                                \
    NODE_ELEMENT(schema).key("INSTRUMENT.DSSC")                                 \
                .commit();                                                      \
                                                                                \
    NODE_ELEMENT(schema).key("INSTRUMENT.DSSC.imageData")                       \
                .commit();                                                      \
                                                                                \
    IMAGEDATA(schema).key("INSTRUMENT.DSSC.imageData.data")                     \
                .setDimensions("128,512")                                       \
                .commit();                                                      \
                                                                                \
    IMAGEDATA(schema).key("INSTRUMENT.DSSC.imageData.asicdata")                 \
                .setDimensions("64,64")                                         \
                .commit();                                                      \
                                                                                \
    SLOT_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData")                       \
                .commit();                                                      \
                                                                                \
    UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.trainId")             \
                .displayedName("trainId")                                       \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    SLOT_ELEMENT(schema).key("INSTRUMENT.DSSC.pulseData")                       \
                .commit();                                                      \
                                                                                \
    VECTOR_UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.pulseData.pulseId")      \
                .displayedName("pulseId")                                       \
                .readOnly().initialValue({0})                                   \
                .commit();                                                      \
                                                                                \

#define XFEL_DAQ_SCHEMA(schema)                                                 \
                                                                                \
        NODE_ELEMENT(data).key("image")                                         \
                .displayedName("Image")                                         \
                .commit();                                                      \
                                                                                \
        NDARRAY_ELEMENT(data).key("image.data")                                 \
                .shape("800,16,64,64")                                          \
                .readOnly()                                                     \
                .commit();                                                      \
                                                                                \
        NDARRAY_ELEMENT(data).key("image.cellId")                               \
                .shape("800")                                                   \
                .readOnly()                                                     \
                .commit();                                                      \
                                                                                \
        NDARRAY_ELEMENT(data).key("image.trainId")                              \
                .shape("800")                                                   \
                .readOnly()                                                     \
                .commit();                                                      \

#define DSSC_TRAINDATA_SCHEMA(schema)                                           \
                                                                                \
    NODE_ELEMENT(schema).key("INSTRUMENT")                                      \
                .commit();                                                      \
                                                                                \
    NODE_ELEMENT(schema).key("INSTRUMENT.DSSC")                                 \
                .commit();                                                      \
                                                                                \
    NODE_ELEMENT(schema).key("INSTRUMENT.DSSC.imageData")                       \
                .commit();                                                      \
                                                                                \
    IMAGEDATA(schema).key("INSTRUMENT.DSSC.imageData.data")                     \
                .commit();                                                      \
                                                                                \
    NODE_ELEMENT(schema).key("INSTRUMENT.DSSC.pulseData")                       \
                .commit();                                                      \
                                                                                \
    VECTOR_UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.pulseData.cellId")       \
                .displayedName("cellId")                                        \
                .readOnly().initialValue({0})                                   \
                .commit();                                                      \
                                                                                \
    VECTOR_UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.pulseData.length")       \
                .displayedName("length")                                        \
                .readOnly().initialValue({0})                                   \
                .commit();                                                      \
                                                                                \
    VECTOR_UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.pulseData.pulseId")      \
                .displayedName("pulseId")                                       \
                .readOnly().initialValue({0})                                   \
                .commit();                                                      \
                                                                                \
    VECTOR_UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.pulseData.status")       \
                .displayedName("status")                                        \
                .readOnly().initialValue({0})                                   \
                .commit();                                                      \
                                                                                \
    VECTOR_UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.pulseData.trainId")      \
                .displayedName("trainId")                                       \
                .readOnly().initialValue({0})                                   \
                .commit();                                                      \
                                                                                \
    NODE_ELEMENT(schema).key("INSTRUMENT.DSSC.specificData")                    \
                .commit();                                                      \
                                                                                \
    VECTOR_UINT8_ELEMENT(schema).key("INSTRUMENT.DSSC.specificData.asicTrailer")\
                .displayedName("asicTrailer")                                   \
                .readOnly().initialValue({0})                                   \
                .commit();                                                      \
                                                                                \
    VECTOR_UINT16_ELEMENT(schema).key("INSTRUMENT.DSSC.specificData.pptData")   \
                .displayedName("pptData")                                       \
                .readOnly().initialValue({0})                                   \
                .commit();                                                      \
                                                                                \
    VECTOR_UINT16_ELEMENT(schema).key("INSTRUMENT.DSSC.specificData.sibData")   \
                .displayedName("sibData")                                       \
                .readOnly().initialValue({0})                                   \
                .commit();                                                      \
                                                                                \
                                                                                \
    NODE_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData")                       \
                .commit();                                                      \
                                                                                \
    UINT64_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.dataId")              \
                .displayedName("dataId")                                        \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    UINT64_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.imageCount")          \
                .displayedName("imageCount")                                    \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.tbLinkId")            \
                .displayedName("tbLinkId")                                      \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.femLinkId")           \
                .displayedName("femLinkId")                                     \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.status")              \
                .displayedName("status")                                        \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    UINT64_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.trainId")             \
                .displayedName("trainId")                                       \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.detSpecificLength")   \
                .displayedName("detSpecificLength")                             \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    UINT64_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.magicNumberStart")    \
                .displayedName("magicNumberStart")                              \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.majorTrainFormatVersion") \
                .displayedName("majorTrainFormatVersion")                       \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    UINT32_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.minorTrainFormatVersion") \
                .displayedName("minorTrainFormatVersion")                       \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    UINT64_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.checkSum0")           \
                .displayedName("checkSum0")                                     \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    UINT64_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.checkSum1")           \
                .displayedName("checkSum1")                                     \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                                                                                \
    UINT64_ELEMENT(schema).key("INSTRUMENT.DSSC.trainData.magicNumberEnd")      \
                .displayedName("magicNumberEnd")                                \
                .readOnly().initialValue(0)                                     \
                .commit();                                                      \
                

#endif	/* DSSCTRAINDATASCHEMA_H */

