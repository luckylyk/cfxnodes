
#ifndef DEF_MESHMATCH
#define DEF_MESHMATCH


#include <maya/MPxDeformerNode.h>
#include <maya/MPxCommand.h>


class MeshMatch : public MPxDeformerNode {
    public:
        MeshMatch();
        virtual ~MeshMatch();
        static void* creator();
        static MStatus initialize();
        virtual MStatus deform(MDataBlock& dataBlock, MItGeometry& vertIter, const MMatrix& matrix, UINT multiIndex);
        void buildMatches(MDataBlock& dataBlock, MObject& inputMesh);
        static MTypeId id;
        static MObject aMeshes;
        static MObject aMatchesGeoIndex;
        static MObject aMatchesVertexId;
        static MObject hasToRebuiltMatches;
};


class MeshMatchCommand: public MPxCommand
    {
    public:
        MeshMatchCommand();
        ~MeshMatchCommand() override;
        MStatus doIt(const MArgList& args) override;
        static MSyntax createSyntax();
        static void* creator();
};

#endif