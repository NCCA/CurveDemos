import maya.OpenMaya as OM
import maya.OpenMayaAnim as OMA
import maya.cmds as cmds


def exportCurves() :
	basicFilter = "*.curve"
	file=cmds.fileDialog2(caption="Please select file to save",fileFilter=basicFilter, dialogStyle=2)
	if file !="" :

		# get the currently selected objects and make sure we have only one object
		selected = OM.MSelectionList()
		OM.MGlobal.getActiveSelectionList(selected)
		selectedObjects = []
		selected.getSelectionStrings(selectedObjects)
		if len(selectedObjects) == 0 :
			cmds.confirmDialog( title='No objects Selected', message='Select a Mesh Object', button=['Ok'], defaultButton='Ok', cancelButton='Ok', dismissString='Ok' )
		else :
			ofile=open(file[0],'w')

			for object in selectedObjects :
				obj=OM.MObject( )
				selected.add(object)
				selected.getDependNode(0,obj)
				fn = OM.MFnTransform(obj)
				curve=""
				oChild = fn.child(0)
				if(oChild.apiTypeStr()=="kNurbsCurve") :
					print "got curve",object
					points=cmds.xform( (object+".cv[*]"), q=True, ws=True, t=True,os=True )
					ofile.write("Curve %s %d\n" %(object, len(points)/3)  )
					for n in range(0,len(points),3) :
						ofile.write("P %f %f %f \n" %(points[n],points[n+1],points[n+2]))
			ofile.close()
