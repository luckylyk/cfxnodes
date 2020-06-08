cmds.file(new=True, f=True)
cmds.unloadPlugin('tension')
cmds.loadPlugin('D:/Works/Python/GitHub/cfxnodes/tension.py')

cube1 = cmds.listRelatives(cmds.polyCube())[0]
cube2 = cmds.listRelatives(cmds.polyCube())[0]
tension = cmds.createNode("meshTensionVertexMap")
cmds.connectAttr(cube1 + ".outMesh", tension + '.mesh')
cmds.connectAttr(cube2 + ".outMesh", tension + '.referenceMesh')

print cmds.getAttr(tension + '.vertexMapOutput')
