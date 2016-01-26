#include "XbimConvert.h"
#include "XbimCurve.h"
#include "XbimCurve2d.h"
#include <gp_Dir2d.hxx>
#include <gp_Dir.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Vec2d.hxx>
#include <gp_XYZ.hxx>
#include <gp_XY.hxx>

#include <gp_Trsf.hxx> 
#include <gp_Trsf2d.hxx> 
#include <gp_Ax2.hxx>
#include <gp_Ax2d.hxx>
#include <gp_Ax3.hxx>
#include <gp_Mat2d.hxx>

using namespace System;
using namespace System::Linq;
using namespace Xbim::Common::Geometry;
using namespace Xbim::Ifc4::Interfaces;

namespace Xbim
{
	namespace Geometry
	{
		XbimConvert::XbimConvert(void)
		{
		}

		// Converts an ObjectPlacement into a TopLoc_Location
		TopLoc_Location XbimConvert::ToLocation(IIfcObjectPlacement^ placement)
		{
			XbimMatrix3D m3D = ConvertMatrix3D(placement);
			gp_Trsf trsf = XbimConvert::ToTransform(m3D);
			return TopLoc_Location(trsf);
		}


		// Converts an Axis2Placement3D into a TopLoc_Location
		TopLoc_Location XbimConvert::ToLocation(IIfcPlacement^ placement)
		{
			if (dynamic_cast<IIfcAxis2Placement3D^>(placement))
			{
				IIfcAxis2Placement3D^ axis3D = (IIfcAxis2Placement3D^)placement;
				return ToLocation(axis3D);
			}
			else if (dynamic_cast<IIfcAxis2Placement2D^>(placement))
			{
				IIfcAxis2Placement2D^ axis2D = (IIfcAxis2Placement2D^)placement;
				return ToLocation(axis2D);
			}
			else
			{
				throw(gcnew NotImplementedException("XbimConvert. Unsupported Placement type, need to implement Grid Placement"));
			}
		}



		TopLoc_Location XbimConvert::ToLocation(Xbim::Ifc4::GeometryResource::IfcAxis2Placement^ placement)
		{
			if (dynamic_cast<IIfcAxis2Placement3D^>(placement))
			{
				IIfcAxis2Placement3D^ axis3D = (IIfcAxis2Placement3D^)placement;
				return ToLocation(axis3D);
			}
			else if (dynamic_cast<IIfcAxis2Placement2D^>(placement))
			{
				IIfcAxis2Placement2D^ axis2D = (IIfcAxis2Placement2D^)placement;
				return ToLocation(axis2D);
			}
			else if (placement == nullptr)
				return TopLoc_Location();
			else
			{
				throw(gcnew NotImplementedException("XbimConvert. Unsupported Placement type, need to implement Grid Placement"));
			}

		}


		TopLoc_Location XbimConvert::ToLocation(IIfcAxis2Placement3D^ axis3D)
		{
			gp_Trsf trsf;
			trsf.SetTransformation(ToAx3(axis3D), gp_Ax3());
			return TopLoc_Location(trsf);
		}

		gp_Ax3 XbimConvert::ToAx3(IIfcAxis2Placement3D^ axis3D)
		{
			gp_XYZ loc(axis3D->Location->X, axis3D->Location->Y, axis3D->Location->Z);
			if (axis3D->Axis != nullptr && axis3D->RefDirection != nullptr) //if one or other is null then use default axis (Ifc Rule)
			{
				gp_Dir zDir(axis3D->Axis->X, axis3D->Axis->Y, axis3D->Axis->Z);
				gp_Dir xDir(axis3D->RefDirection->X, axis3D->RefDirection->Y, axis3D->RefDirection->Z);
				return gp_Ax3(gp_Ax2(loc, zDir, xDir));
			}
			else
			{
				gp_Dir zDir(0, 0, 1);
				gp_Dir xDir(1, 0, 0);
				return gp_Ax3(gp_Ax2(loc, zDir, xDir));
			}
		}



		TopLoc_Location XbimConvert::ToLocation(IIfcAxis2Placement2D^ axis2D)
		{
			gp_Pnt2d loc(axis2D->Location->X, axis2D->Location->Y);

			// If problems with creation of direction occur default direction is used
			gp_Dir2d Vxgp = gp_Dir2d(1., 0.);
			if (axis2D->RefDirection != nullptr)
			{
				Standard_Real X = axis2D->RefDirection->X;
				Standard_Real Y = axis2D->RefDirection->Y;

				//Direction is not created if it has null magnitude
				gp_XY aXY(X, Y);
				Standard_Real res2 = gp::Resolution()*gp::Resolution();
				Standard_Real aMagnitude = aXY.SquareModulus();
				if (aMagnitude > res2)
				{
					Vxgp = gp_Dir2d(aXY);
				}

			}
			gp_Ax2d axis2d(loc, Vxgp);
			gp_Trsf2d trsf;
			trsf.SetTransformation(axis2d);
			trsf.Invert();
			return TopLoc_Location(trsf);
		}

		gp_Pln XbimConvert::ToPlane(IIfcAxis2Placement3D^ axis3D)
		{
			return gp_Pln(ToAx3(axis3D));
		}


		gp_Trsf XbimConvert::ToTransform(IIfcAxis2Placement3D^ axis3D)
		{
			gp_Trsf trsf;
			trsf.SetTransformation(ToAx3(axis3D), gp_Ax3(gp_Pnt(), gp_Dir(0, 0, 1), gp_Dir(1, 0, 0)));
			return trsf;
		}

		gp_Trsf XbimConvert::ToTransform(IIfcCartesianTransformationOperator^ tForm)
		{
			if (dynamic_cast<IIfcCartesianTransformationOperator3DnonUniform^>(tForm))
				//Call the special case method for non uniform transforms and use BRepBuilderAPI_GTransform
				//instead of BRepBuilderAPI_Transform,  see opencascade issue
				// http://www.opencascade.org/org/forum/thread_300/?forum=3
				throw (gcnew XbimGeometryException("XbimConvert. IfcCartesianTransformationOperator3DnonUniform require a specific call"));
			else if (dynamic_cast<IIfcCartesianTransformationOperator3D^>(tForm))
				return ToTransform((IIfcCartesianTransformationOperator3D^)tForm);
			else if (dynamic_cast<IIfcCartesianTransformationOperator2D^>(tForm))
				return ToTransform((IIfcCartesianTransformationOperator2D^)tForm);
			else if (tForm == nullptr)
				return gp_Trsf();
			else
				throw(gcnew ArgumentOutOfRangeException("XbimConvert. Unsupported CartesianTransformationOperator type"));
		}

		gp_Trsf XbimConvert::ToTransform(IIfcCartesianTransformationOperator3D^ ct3D)
		{
			XbimVector3D U3; //Z Axis Direction
			XbimVector3D U2; //X Axis Direction
			XbimVector3D U1; //Y axis direction
			if (ct3D->Axis3 != nullptr)
			{
				IIfcDirection^ dir = ct3D->Axis3;
				U3 = XbimVector3D(dir->X, dir->Y, dir->Z);
				U3.Normalize();
			}
			else
				U3 = XbimVector3D(0., 0., 1.);
			if (ct3D->Axis1 != nullptr)
			{
				IIfcDirection^ dir = ct3D->Axis1;
				U1 = XbimVector3D(dir->X, dir->Y, dir->Z);
				U1.Normalize();
			}
			else
			{
				XbimVector3D defXDir(1., 0., 0.);
				if (U3 != defXDir)
					U1 = defXDir;
				else
					U1 = XbimVector3D(0., 1., 0.);
			}
			XbimVector3D xVec = XbimVector3D::Multiply(XbimVector3D::DotProduct(U1, U3), U3);
			XbimVector3D xAxis = XbimVector3D::Subtract(U1, xVec);
			xAxis.Normalize();

			if (ct3D->Axis2 != nullptr)
			{
				IIfcDirection^ dir = ct3D->Axis2;
				U2 = XbimVector3D(dir->X, dir->Y, dir->Z);
				U2.Normalize();
			}
			else
				U2 = XbimVector3D(0., 1., 0.);

			XbimVector3D tmp = XbimVector3D::Multiply(XbimVector3D::DotProduct(U2, U3), U3);
			XbimVector3D yAxis = XbimVector3D::Subtract(U2, tmp);
			tmp = XbimVector3D::Multiply(XbimVector3D::DotProduct(U2, xAxis), xAxis);
			yAxis = XbimVector3D::Subtract(yAxis, tmp);
			yAxis.Normalize();
			U2 = yAxis;
			U1 = xAxis;

			XbimPoint3D LO(ct3D->LocalOrigin->X, ct3D->LocalOrigin->Y, ct3D->LocalOrigin->Z); //local origin

			gp_Trsf trsf;
			trsf.SetValues(U1.X, U1.Y, U1.Z, 0,
				U2.X, U2.Y, U2.Z, 0,
				U3.X, U3.Y, U3.Z, 0);

			trsf.SetTranslationPart(gp_Vec(LO.X, LO.Y, LO.Z));
			trsf.SetScaleFactor(ct3D->Scl);
			return trsf;
		}

		gp_GTrsf XbimConvert::ToTransform(IIfcCartesianTransformationOperator3DnonUniform^ ct3D)
		{
			XbimVector3D U3; //Z Axis Direction
			XbimVector3D U2; //X Axis Direction
			XbimVector3D U1; //Y axis direction
			if (ct3D->Axis3 != nullptr)
			{
				IIfcDirection^ dir = ct3D->Axis3;
				U3 = XbimVector3D(dir->X, dir->Y, dir->Z);
				U3.Normalize();
			}
			else
				U3 = XbimVector3D(0., 0., 1.);
			if (ct3D->Axis1 != nullptr)
			{
				IIfcDirection^ dir = ct3D->Axis1;
				U1 = XbimVector3D(dir->X, dir->Y, dir->Z);
				U1.Normalize();
			}
			else
			{
				XbimVector3D defXDir(1., 0., 0.);
				if (U3 != defXDir)
					U1 = defXDir;
				else
					U1 = XbimVector3D(0., 1., 0.);
			}
			XbimVector3D xVec = XbimVector3D::Multiply(XbimVector3D::DotProduct(U1, U3), U3);
			XbimVector3D xAxis = XbimVector3D::Subtract(U1, xVec);
			xAxis.Normalize();

			if (ct3D->Axis2 != nullptr)
			{
				IIfcDirection^ dir = ct3D->Axis2;
				U2 = XbimVector3D(dir->X, dir->Y, dir->Z);
				U2.Normalize();
			}
			else
				U2 = XbimVector3D(0., 1., 0.);

			XbimVector3D tmp = XbimVector3D::Multiply(XbimVector3D::DotProduct(U2, U3), U3);
			XbimVector3D yAxis = XbimVector3D::Subtract(U2, tmp);
			tmp = XbimVector3D::Multiply(XbimVector3D::DotProduct(U2, xAxis), xAxis);
			yAxis = XbimVector3D::Subtract(yAxis, tmp);
			yAxis.Normalize();
			U2 = yAxis;
			U1 = xAxis;

			XbimPoint3D LO(ct3D->LocalOrigin->X, ct3D->LocalOrigin->Y, ct3D->LocalOrigin->Z); //local origin

			double s1 = ct3D->Scl*U1.X;
			double s2 = ct3D->Scl2* U2.Y;
			double s3 = ct3D->Scl3* U3.Z;
			gp_GTrsf trsf(
				gp_Mat(s1, U1.Y, U1.Z,
				U2.X, s2, U2.Z,
				U3.X, U3.Y, s3
				),
				gp_XYZ(LO.X, LO.Y, LO.Z));

			return trsf;
		}

		gp_Trsf XbimConvert::ToTransform(XbimMatrix3D m3D)
		{
			gp_Trsf trsf;
			trsf.SetValues(m3D.M11, m3D.M21, m3D.M31, m3D.OffsetX,
				m3D.M12, m3D.M22, m3D.M32, m3D.OffsetY,
				m3D.M13, m3D.M23, m3D.M33, m3D.OffsetZ);
			//trsf.SetTranslationPart(gp_Vec(m3D.OffsetX, m3D.OffsetY, m3D.OffsetZ));
			return trsf;
		}

		XbimMatrix3D XbimConvert::ToMatrix3D(const TopLoc_Location& location)
		{
			const gp_Trsf& trsf = location.Transformation();
			gp_Mat m = trsf.VectorialPart();
			gp_XYZ t = trsf.TranslationPart();
			return XbimMatrix3D((double)m.Row(1).X(), (double)m.Row(1).Y(), (double)m.Row(1).Z(), 0.0,
				(double)m.Row(2).X(), (double)m.Row(2).Y(), (double)m.Row(2).Z(), 0.0,
				(double)m.Row(3).X(), (double)m.Row(3).Y(), (double)m.Row(3).Z(), 0.0,
				(double)t.X(), (double)t.Y(), (double)t.Z(), 1.0);
		}

		gp_Trsf XbimConvert::ToTransform(IIfcCartesianTransformationOperator2D^ ct)
		{
			gp_Trsf2d m;
			IIfcDirection^ axis1 = ct->Axis1;
			IIfcDirection^ axis2 = ct->Axis2;
			double scale = ct->Scl;
			IIfcCartesianPoint^ o = ct->LocalOrigin;
			gp_Mat2d mat = m.HVectorialPart();
			if (axis1 != nullptr)
			{
				XbimVector3D d1(axis1->X, axis1->Y, axis1->Z);
				d1.Normalize();
				mat.SetValue(1, 1, d1.X);
				mat.SetValue(1, 2, d1.Y);
				mat.SetValue(2, 1, -d1.Y);
				mat.SetValue(2, 2, d1.X);

				if (axis2 != nullptr)
				{
					XbimVector3D v(-d1.Y, d1.X, 0);
					double factor = XbimVector3D::DotProduct(XbimVector3D(axis2->X, axis2->Y, axis2->Z), v);
					if (factor < 0)
					{
						mat.SetValue(2, 1, d1.Y);
						mat.SetValue(2, 2, -d1.X);
					}
				}
			}
			else
			{
				if (axis2 != nullptr)
				{
					XbimVector3D d1(axis2->X, axis2->Y, axis2->Z);
					d1.Normalize();
					mat.SetValue(1, 1, d1.Y);
					mat.SetValue(1, 2, -d1.X);
					mat.SetValue(2, 1, d1.X);
					mat.SetValue(2, 2, d1.X);
				}
			}

			m.SetScaleFactor(scale);
			m.SetTranslationPart(gp_Vec2d(o->X, o->Y));
			return m;

		}

		// Builds a windows Matrix3D from a CartesianTransformationOperator3D
		XbimMatrix3D XbimConvert::ConvertMatrix3D(IIfcCartesianTransformationOperator3D ^ stepTransform)
		{
			XbimVector3D U3; //Z Axis Direction
			XbimVector3D U2; //X Axis Direction
			XbimVector3D U1; //Y axis direction
			if (stepTransform->Axis3 != nullptr)
			{
				IIfcDirection^ dir = (IIfcDirection^)stepTransform->Axis3;
				U3 = XbimVector3D(dir->X, dir->Y, dir->Z);
				U3.Normalize();
			}
			else
				U3 = XbimVector3D(0., 0., 1.);
			if (stepTransform->Axis1 != nullptr)
			{
				IIfcDirection^ dir = (IIfcDirection^)stepTransform->Axis1;
				U1 = XbimVector3D(dir->X, dir->Y, dir->Z);
				U1.Normalize();
			}
			else
			{
				XbimVector3D defXDir(1., 0., 0.);
				if (U3 != defXDir)
					U1 = defXDir;
				else
					U1 = XbimVector3D(0., 1., 0.);
			}
			XbimVector3D xVec = XbimVector3D::Multiply(XbimVector3D::DotProduct(U1, U3), U3);
			XbimVector3D xAxis = XbimVector3D::Subtract(U1, xVec);
			xAxis.Normalize();

			if (stepTransform->Axis2 != nullptr)
			{
				IIfcDirection^ dir = (IIfcDirection^)stepTransform->Axis2;
				U2 = XbimVector3D(dir->X, dir->Y, dir->Z);
				U2.Normalize();
			}
			else
				U2 = XbimVector3D(0., 1., 0.);

			XbimVector3D tmp = XbimVector3D::Multiply(XbimVector3D::DotProduct(U2, U3), U3);
			XbimVector3D yAxis = XbimVector3D::Subtract(U2, tmp);
			tmp = XbimVector3D::Multiply(XbimVector3D::DotProduct(U2, xAxis), xAxis);
			yAxis = XbimVector3D::Subtract(yAxis, tmp);
			yAxis.Normalize();
			U2 = yAxis;
			U1 = xAxis;

			XbimPoint3D LO(stepTransform->LocalOrigin->X, stepTransform->LocalOrigin->Y, stepTransform->LocalOrigin->Z); //local origin
			float S = 1.;
			if (stepTransform->Scale.HasValue)
				S = (float)stepTransform->Scale.Value;

			return XbimMatrix3D(U1.X, U1.Y, U1.Z, 0,
				U2.X, U2.Y, U2.Z, 0,
				U3.X, U3.Y, U3.Z, 0,
				LO.X, LO.Y, LO.Z, S);
		}

		// Builds a windows Matrix3D from an ObjectPlacement
		XbimMatrix3D XbimConvert::ConvertMatrix3D(IIfcObjectPlacement ^ objPlacement)
		{
			if (dynamic_cast<IIfcLocalPlacement^>(objPlacement))
			{
				IIfcLocalPlacement^ locPlacement = (IIfcLocalPlacement^)objPlacement;
				if (dynamic_cast<IIfcAxis2Placement3D^>(locPlacement->RelativePlacement))
				{
					XbimMatrix3D ucsTowcs =ToMatrix3D((IIfcAxis2Placement3D^)(locPlacement->RelativePlacement));
					if (locPlacement->PlacementRelTo != nullptr)
					{
						return XbimMatrix3D::Multiply(ConvertMatrix3D(locPlacement->PlacementRelTo), ucsTowcs);
					}
					else
						return ucsTowcs;

				}
				else //must be 2D
				{
					throw(gcnew System::NotImplementedException("Support for Placements other than 3D not implemented"));
				}

			}
			else if (dynamic_cast<IIfcGridPlacement^>(objPlacement)) // a Grid
			{
				IIfcGridPlacement^ gridPlacement = (IIfcGridPlacement^)objPlacement;

				IIfcVirtualGridIntersection^ vi = gridPlacement->PlacementLocation;
				List<IIfcGridAxis^>^ axises = Enumerable::ToList(vi->IntersectingAxes);
				double tolerance = vi->Model->ModelFactors->Precision;
				//its 2d, it should always be		
				XbimCurve2D^ axis1 = gcnew XbimCurve2D(axises[0]);
				XbimCurve2D^ axis2 = gcnew XbimCurve2D(axises[1]);
				IEnumerable<XbimPoint3D>^ intersects = axis1->Intersections(axis2, tolerance);
				if (!Enumerable::Any(intersects)) return XbimMatrix3D::Identity;
				
			    XbimPoint3D intersection = Enumerable::First(intersects);
				gp_Ax2d ax;

				if (gridPlacement->PlacementRefDirection==nullptr)
				{
					double p1 = axis1->GetParameter(intersection, tolerance);					
					XbimVector3D v = axis1->TangentAt(p1);
					ax.SetDirection(gp_Dir2d(v.X,v.Y));
				}
				else if (dynamic_cast<IIfcDirection^>(gridPlacement->PlacementRefDirection))
				{
					ax.SetDirection(XbimConvert::GetDir2d((IIfcDirection^)gridPlacement->PlacementRefDirection));
				}
				else if (dynamic_cast<IIfcVirtualGridIntersection^>(gridPlacement->PlacementRefDirection))
				{
					IIfcVirtualGridIntersection^ v2 = (IIfcVirtualGridIntersection^)gridPlacement->PlacementRefDirection;
					List<IIfcGridAxis^>^ axisesv2 = Enumerable::ToList(v2->IntersectingAxes);
					//its 2d, it should always be		
					XbimCurve2D^ axis1v = gcnew XbimCurve2D(axisesv2[0]);
					XbimCurve2D^ axis2v = gcnew XbimCurve2D(axisesv2[1]);
					IEnumerable<XbimPoint3D>^ intersectsv = axis1v->Intersections(axis2v, tolerance);

					XbimPoint3D intersectionv = Enumerable::First(intersectsv);
					XbimVector3D vec2 = intersectionv - intersection;
					ax.SetDirection(gp_Dir2d(vec2.X, vec2.Y));
				}

				gp_Vec v = XbimConvert::GetDir3d(vi->OffsetDistances); //go for 3D
				ax.SetLocation(gp_Pnt2d(intersection.X, intersection.Y));
				gp_XY xy(v.X(), v.Y());
				gp_Trsf2d tr;
				tr.SetTransformation(ax);
				tr.Transforms(xy);

				intersection = XbimPoint3D(xy.X(), xy.Y(), v.Z());
				XbimMatrix3D localTrans = XbimMatrix3D::CreateTranslation(intersection.X, intersection.Y, intersection.Z);
				//now adopt the placement of the grid, this is not performant
				IIfcGrid^ grid = Enumerable::FirstOrDefault(axises[0]->PartOfU);
				if (grid == nullptr) grid = Enumerable::FirstOrDefault(axises[0]->PartOfV);
				if (grid == nullptr) grid = Enumerable::FirstOrDefault(axises[0]->PartOfW);
				//we must have one now
				XbimMatrix3D gridTransform = ConvertMatrix3D(grid->ObjectPlacement);
				return XbimMatrix3D::Multiply(localTrans, gridTransform);

			}

		}



		XbimMatrix3D XbimConvert::ToMatrix3D(IIfcAxis2Placement3D^ axis3)
		{
			
			if (axis3->RefDirection != nullptr && axis3->Axis != nullptr)
			{
				XbimVector3D za(axis3->Axis->X, axis3->Axis->Y, axis3->Axis->Z) ;
				za.Normalize();
				XbimVector3D xa(axis3->RefDirection->X, axis3->RefDirection->Y, axis3->RefDirection->Z);
				xa.Normalize();
				XbimVector3D ya = XbimVector3D::CrossProduct(za, xa);
				ya.Normalize();
				return  XbimMatrix3D(xa.X, xa.Y, xa.Z, 0, ya.X, ya.Y, ya.Z, 0, za.X, za.Y, za.Z, 0, axis3->Location->X,
					axis3->Location->Y, axis3->Location->Z, 1);
			}
			else
				return  XbimMatrix3D(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, axis3->Location->X, axis3->Location->Y,
				axis3->Location->Z, 1);
		}


		bool XbimConvert::IsEqual(IIfcCartesianPoint^ ptA, IIfcCartesianPoint^ ptB, double tolerance)
		{
			return XbimConvert::DistanceSquared(ptA, ptB) <= (tolerance * tolerance);
		}

		double XbimConvert::DistanceSquared(IIfcCartesianPoint^ pt1, IIfcCartesianPoint^ pt2)
		{
			double d = 0, dd;
			double x1 = pt1->X;
			double y1 = pt1->Y;
			double z1 = GetZValueOrZero(pt1);
			double x2 = pt2->X;
			double y2 = pt2->Y;
			double z2 = GetZValueOrZero(pt2);
			dd = x1; dd -= x2; dd *= dd; d += dd;
			dd = y1; dd -= y2; dd *= dd; d += dd;
			dd = z1; dd -= z2; dd *= dd; d += dd;
			return d;
		}

		double  XbimConvert::GetZValueOrZero(IIfcCartesianPoint^ point)
		{			
			if (point->Dim == dimensions3D) return point->Z; else return 0.0;
		}

		double  XbimConvert::GetZValueOrZero(IIfcDirection^ dir)
		{
			if (dir->Dim == dimensions3D) return dir->Z; else return 0.0;
		}

		double  XbimConvert::GetZValueOrZero(IIfcVector^ vec)
		{
			if (vec->Dim == dimensions3D) return vec->Orientation->Z; else return 0.0;
		}

		gp_Pnt XbimConvert::GetPoint3d(IIfcCartesianPoint^ cartesian)
		{
			return gp_Pnt(cartesian->X, cartesian->Y, XbimConvert::GetZValueOrZero(cartesian));
		}

		gp_Pnt2d XbimConvert::GetPoint2d(IIfcCartesianPoint^ cartesian)
		{
			return gp_Pnt2d(cartesian->X, cartesian->Y);
		}

		bool  XbimConvert::Is3D(IIfcPolyline^ pline)
		{
			for each (IIfcCartesianPoint^ pt in pline->Points)
				return Enumerable::Count(pt->Coordinates) == 3;
			return false;
		}

		bool  XbimConvert::Is3D(IIfcPolyLoop^ pLoop)
		{
			for each (IIfcCartesianPoint^ pt in pLoop->Polygon)
				return Enumerable::Count(pt->Coordinates) == 3;
			return false;
		}
		bool  XbimConvert::IsPolygon(IIfcPolyLoop^ pLoop)
		{
			int sides = 0;
			for each (IIfcCartesianPoint^ pt in pLoop->Polygon)
			{
				sides++;
				if (sides > 2)return true;
			}
			return false;
		}

		/// <summary>
		/// Calculates the Newell's Normal of the polygon of the loop
		/// </summary>
		/// <param name="loop"></param>
		/// <returns></returns>
		XbimVector3D XbimConvert::NewellsNormal(IIfcPolyLoop^ loop)
		{
			double x = 0, y = 0, z = 0;
			IIfcCartesianPoint^ previous = nullptr;
			int count = 0;
			List<IIfcCartesianPoint^>^ polygon = Enumerable::ToList(loop->Polygon);
			int total = polygon->Count;
			for (int i = 0; i <= total; i++)
			{
				IIfcCartesianPoint^ current = i < total ? polygon[i] : polygon[0];
				if (count > 0)
				{
					double xn = previous->X;
					double yn = previous->Y;
					double zn = previous->Z;
					double xn1 = current->X;
					double yn1 = current->Y;
					double zn1 = current->Z;
					x += (yn - yn1) * (zn + zn1);
					y += (xn + xn1) * (zn - zn1);
					z += (xn - xn1) * (yn + yn1);
				}
				previous = current;
				count++;
			}
			XbimVector3D v(x, y, z);
			v.Normalize();
			return v;
		}

		gp_Dir XbimConvert::GetDir3d(IIfcDirection^ dir)
		{
			return gp_Dir(dir->X, dir->Y, XbimConvert::GetZValueOrZero(dir));
		}

		gp_Dir2d XbimConvert::GetDir2d(IIfcDirection^ dir)
		{
			return gp_Dir2d(dir->X, dir->Y);
		}

		gp_Vec2d XbimConvert::GetDir2d(IEnumerable<IfcLengthMeasure>^ offsets)
		{
			gp_Vec2d v;
			IEnumerator<IfcLengthMeasure>^ enumer = offsets->GetEnumerator();
			if (enumer->MoveNext()) v.SetX(enumer->Current); else v.SetX(0.);
			if (enumer->MoveNext()) v.SetY(enumer->Current); else v.SetY(0.);
			return v;
		}
		gp_Vec XbimConvert::GetDir3d(IEnumerable<IfcLengthMeasure>^ offsets)
		{
			gp_Vec v;
			IEnumerator<IfcLengthMeasure>^ enumer = offsets->GetEnumerator();
			if (enumer->MoveNext()) v.SetX(enumer->Current); else v.SetX(0.);
			if (enumer->MoveNext()) v.SetY(enumer->Current); else v.SetY(0.);
			if (enumer->MoveNext()) v.SetZ(enumer->Current); else v.SetZ(0.);
			return v;
		}
	}
}