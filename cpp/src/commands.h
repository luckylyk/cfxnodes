
#ifndef DEF_CNC_COMMAND
#define DEF_CNC_COMMAND

#include <maya/MPxCommand.h>


class MeshMatchCommand: public MPxCommand
    {
    public:
        MeshMatchCommand();
        ~MeshMatchCommand() override;
        MStatus doIt(const MArgList& args) override;
        static MSyntax createSyntax();
        static void* creator();
        static MString name;
};


#endif