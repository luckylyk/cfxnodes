
# tension map test
cube1 = cmds.listRelatives(cmds.polyCube())[0]
cube2 = cmds.listRelatives(cmds.polyCube())[0]
tension = cmds.createNode("meshTensionVertexMap")
cmds.connectAttr(cube1 + ".outMesh", tension + '.mesh')
cmds.connectAttr(cube2 + ".outMesh", tension + '.referenceMesh')
print cmds.getAttr(tension + '.vertexMapOutput')


# mesh delta test
cmds.loadPlugin('D:\Works\Python\GitHub\cfxnodes\meshdelta.py')
cube = cmds.polyCube()[0]
cube2 = cmds.polyCube()[0]
cube3 = cmds.polyCube()[0]
cmds.select(cube2)
cmds.move(2, 0, 0)
cmds.select(cube3)
cmds.move(4, 0, 0)
cmds.select(cube2 + '.vtx[0]')
cmds.move(0, 0, .2, r=True)
cmds.select(cube3 + '.vtx[1:2]')
cmds.move(0.2, 0, 0, r=True)
cmds.select(cube)
deformer = cmds.deformer(type="meshDelta")[0]
cmds.connectAttr(cube2[:-1] + 'Shape2.outMesh', deformer + '.referenceMesh')
cmds.connectAttr(cube3[:-1] + 'Shape3.outMesh', deformer + '.offsetMesh')