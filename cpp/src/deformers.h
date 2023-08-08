
#ifndef DEF_DEFORMERS
#define DEF_DEFORMERS

#include <vector>
#include <maya/MPxDeformerNode.h>


class Magnet : public MPxDeformerNode {
    public:
        Magnet();
        virtual ~Magnet();
        static void* creator();
        static MStatus initialize();
        virtual MStatus deform(MDataBlock& dataBlock, MItGeometry& vertIter, const MMatrix& matrix, UINT multiIndex);
        static MString name;
        static MTypeId id;
        static MObject magnetMesh;
        static MObject interpolation;
        static MObject minInfluenceRadius;
        static MObject maxInfluenceRadius;
        static MObject normalAngleInfluence;
        static MObject minNormalInfluence;
        static MObject maxNormalInfluence;
        static MObject offset;

        std::vector<std::vector<int>> neightbourOfVertices;
        int backedInterpolation;
};


class MeshDelta : public MPxDeformerNode {
    public:
        MeshDelta();
        virtual ~MeshDelta();
        static void* creator();
        static MStatus initialize();
        virtual MStatus deform(MDataBlock& dataBlock, MItGeometry& vertIter, const MMatrix& matrix, UINT multiIndex);
        static MString name;
        static MTypeId id;
        static MObject referenceMesh;
        static MObject offsetMesh;
};


class Pooth : public MPxDeformerNode {
    public:
        Pooth();
        virtual ~Pooth();
        static void* creator();
        static MStatus initialize();
        virtual MStatus deform(MDataBlock& dataBlock, MItGeometry& vertIter, const MMatrix& matrix, UINT multiIndex);
        static MString name;
        static MTypeId id;
        static MObject aPush;
        static MObject aSmoothEnvelope;
        static MObject aSmoothStep;
        static MObject aStepIteration;

        std::vector<std::vector<int>> neightbourOfVertices;
        int backedStepValue;
};


class PushOut: public MPxDeformerNode {
    public:
        PushOut();
        virtual ~PushOut();
        static void* creator();
        static MStatus initialize();
        virtual MStatus deform(MDataBlock& dataBlock, MItGeometry& vertIter, const MMatrix& matrix, UINT multiIndex);
        static MString name;
        static MTypeId id;
        static MObject interpolation;
        static MObject aPusher;
        static MObject aPush;
        static MObject aRadiusOffset;
        static MObject aDistanceMax;

        std::vector<std::vector<int>> neightbourOfVertices;
        int backedInterpolation;
};


#endif