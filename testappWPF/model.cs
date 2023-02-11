using glTFLoader.Schema;
using glTFLoader;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media.Media3D;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.IO;
using System.Threading.Channels;
using System.IO.Enumeration;

namespace testappWPF
{
  /// <summary>
  /// Uses the glTF2Loader NuGet component to build a System.Windows.Media.Media3D.Model3DGroup.
  /// This model is then used to provide a (basic) 3D preview of the glTF file in the Viewport3D control.
  /// </summary>
  internal class Model
  {
    public Model( string filename )
    {
      try {
        modelFilepath = filename;
        var model = Interface.LoadModel( filename );
        scenes = new List<Model3DGroup>();
        foreach ( var scene in model.Scenes ) {
          var model3DGroup = new Model3DGroup();
          ReadScene( model, scene, model3DGroup );
          scenes.Add( model3DGroup );
        }
      } catch ( Exception ex ) {
        #if DEBUG
          System.Diagnostics.Debug.WriteLine( "Failed to load model: " + ex.Message );
        #endif
      }
    }

    public void Close()
    {
      foreach( var file in tempTextureFiles) {
        System.IO.File.Delete( file );
      }
    }

    private void ReadScene( Gltf model, Scene scene, Model3DGroup model3DGroup )
    {
      string name = scene.Name;
      Matrix3D matrix = Matrix3D.Identity;

      foreach ( var node in scene.Nodes ) {
        ReadNode( model, model.Nodes[ node ], matrix, model3DGroup );
      }
    }

    private void ReadNode( Gltf model, Node node, Matrix3D matrix, Model3DGroup model3DGroup )
    {
      var translation = node.Translation;
      var rotation = node.Rotation;
      var scale = node.Scale;

      bool hasTRS = ( translation.Count() == 3 ) && ( ( translation[ 0 ] != 0 ) || ( translation[ 1 ] != 0 ) || ( translation[ 2 ] != 0 ) );
      hasTRS |= ( rotation.Count() == 4 ) && ( ( rotation[ 0 ] != 0 ) || ( rotation[ 1 ] != 0 ) || ( rotation[ 2 ] != 0 ) || ( rotation[ 3 ] != 1 ) );
      hasTRS |= ( scale.Count() == 3 ) && ( ( scale[ 0 ] != 1 ) || ( scale[ 1 ] != 1 ) || ( scale[ 2 ] != 1 ) );

      Matrix3D nodeMatrix = Matrix3D.Identity;

      if ( hasTRS ) {
        var translationMatrix = Matrix3D.Identity;
        translationMatrix.Translate( new Vector3D( translation[ 0 ], translation[ 1 ], translation[ 2 ] ) );

        var rotationMatrix = Matrix3D.Identity;
        rotationMatrix.Rotate( new Quaternion( rotation[ 0 ], rotation[ 1 ], rotation[ 2 ], rotation[ 3 ] ) );

        var scaleMatrix = Matrix3D.Identity;
        scaleMatrix.Scale( new Vector3D( scale[ 0 ], scale[ 1 ], scale[ 2 ] ) );

        nodeMatrix = scaleMatrix * rotationMatrix * translationMatrix;
      } else if ( node.Matrix.Count() == 16 ) {
        nodeMatrix = new Matrix3D(
          node.Matrix[ 0 ], node.Matrix[ 1 ], node.Matrix[ 2 ], node.Matrix[ 3 ], 
          node.Matrix[ 4 ], node.Matrix[ 5 ], node.Matrix[ 6 ], node.Matrix[ 7 ], 
          node.Matrix[ 8 ], node.Matrix[ 9 ], node.Matrix[ 10 ], node.Matrix[ 11 ], 
          node.Matrix[ 12 ], node.Matrix[ 13 ], node.Matrix[ 14 ], node.Matrix[ 15 ] );
      }
      nodeMatrix *= matrix;

      if ( node.Mesh.HasValue ) {
        ReadMesh( model, model.Meshes[ node.Mesh.Value ], nodeMatrix, model3DGroup );
      }

      if ( node.Children != null ) {
        foreach( var child in node.Children ) {
          ReadNode( model, model.Nodes[ child ], nodeMatrix, model3DGroup );
        }
      }
    }

    private void ReadMesh( Gltf model, Mesh mesh, Matrix3D matrix, Model3DGroup model3DGroup )
    {
      foreach ( var primitive in mesh.Primitives ) {
        // Only handle triangle mesh primitives
        if ( MeshPrimitive.ModeEnum.TRIANGLES == primitive.Mode ) {
          // A mesh should at least contain position values
          if ( primitive.Attributes.ContainsKey( "POSITION" ) ) {
            var positionAttribute = primitive.Attributes[ "POSITION" ];
            Point3DCollection? positions = ReadPoints( model, model.Accessors[ positionAttribute ] );

            Vector3DCollection? normals = null;
            if ( primitive.Attributes.ContainsKey( "NORMAL" ) ) {
              var normalAttribute = primitive.Attributes[ "NORMAL" ];
              normals = ReadVectors( model, model.Accessors[ normalAttribute ] );
            }

            PointCollection? texcoords = null;
            if ( primitive.Attributes.ContainsKey( "TEXCOORD_0" ) ) {
              // Only use the first texture coordinate channel.
              var texcoordAttribute = primitive.Attributes[ "TEXCOORD_0" ];
              texcoords = ReadTexcoords( model, model.Accessors[ texcoordAttribute ] );
            }

            Int32Collection? indices = null;
            if ( primitive.Indices.HasValue ) {
              // Indexed mesh
              indices = ReadScalar( model, model.Accessors[ primitive.Indices.Value ] );
            } else if ( null != positions ) {
              // Non-indexed mesh
              indices = new Int32Collection();
              for ( int i = 0; i < positions.Count; i++ ) {
                indices.Add( i );
              }
            }

            if ( ( indices != null ) && ( positions != null ) ) {
              MeshGeometry3D meshGeometry = new MeshGeometry3D();
              meshGeometry.TriangleIndices = indices;
              foreach ( var point in positions ) {
                meshGeometry.Positions.Add( matrix.Transform( point ) );
              }
              if ( null != normals ) {
                foreach ( var normal in normals ) {
                  meshGeometry.Normals.Add( matrix.Transform( normal ) );
                }
              }
              if ( null != texcoords ) {
                meshGeometry.TextureCoordinates = texcoords;
              }
              GeometryModel3D geometry = new GeometryModel3D( meshGeometry, GetMaterial( model, primitive ) );
              geometry.BackMaterial = geometry.Material;
              model3DGroup.Children.Add( geometry );
            }
          }
        }
      }
    }

    private Vector3DCollection? ReadVectors( Gltf model, Accessor accessor )
    {
      Vector3DCollection? result = null;
      // TODO support non-float source data, sparse accessors, and validate count, byte length, byte offset, etc.
      if ( ( Accessor.TypeEnum.VEC3 == accessor.Type ) && ( Accessor.ComponentTypeEnum.FLOAT == accessor.ComponentType ) && accessor.BufferView.HasValue && ( accessor.Count > 0 ) && ( null == accessor.Sparse ) ) {
        var bufferView = model.BufferViews[ accessor.BufferView.Value ];
        var buffer = model.LoadBinaryBuffer( bufferView.Buffer, modelFilepath );
        result = new Vector3DCollection();
        int sourceOffset = accessor.ByteOffset + bufferView.ByteOffset;
        int stride = bufferView.ByteStride.HasValue ? bufferView.ByteStride.Value : 12;
        for ( int i = 0; i < accessor.Count; i++ ) {
          float x = BitConverter.ToSingle( buffer, sourceOffset );
          float y = BitConverter.ToSingle( buffer, sourceOffset + 4 );
          float z = BitConverter.ToSingle( buffer, sourceOffset + 8 );
          result.Add( new Vector3D( x, y, z ) );
          sourceOffset += stride;
        }        
      }
      return result;
    }

    private Point3DCollection? ReadPoints( Gltf model, Accessor accessor )
    {
      Point3DCollection? result = null;
      // TODO support non-float source data, sparse accessors, and validate count, byte length, byte offset, etc.
      if ( ( Accessor.TypeEnum.VEC3 == accessor.Type ) && ( Accessor.ComponentTypeEnum.FLOAT == accessor.ComponentType ) && accessor.BufferView.HasValue && ( accessor.Count > 0 ) && ( null == accessor.Sparse ) ) {
        var bufferView = model.BufferViews[ accessor.BufferView.Value ];
        var buffer = model.LoadBinaryBuffer( bufferView.Buffer, modelFilepath );
        result = new Point3DCollection();
        int sourceOffset = accessor.ByteOffset + bufferView.ByteOffset;
        int stride = bufferView.ByteStride.HasValue ? bufferView.ByteStride.Value : 12;
        for ( int i = 0; i < accessor.Count; i++ ) {
          float x = BitConverter.ToSingle( buffer, sourceOffset );
          float y = BitConverter.ToSingle( buffer, sourceOffset + 4 );
          float z = BitConverter.ToSingle( buffer, sourceOffset + 8 );
          result.Add( new Point3D( x, y, z ) );
          sourceOffset += stride;
        }        
      }
      return result;
    }

    private PointCollection? ReadTexcoords( Gltf model, Accessor accessor )
    {
      PointCollection? result = null;
      // TODO support non-float source data, sparse accessors, and validate count, byte length, byte offset, etc.
      if ( ( Accessor.TypeEnum.VEC2 == accessor.Type ) && ( Accessor.ComponentTypeEnum.FLOAT == accessor.ComponentType ) && accessor.BufferView.HasValue && ( accessor.Count > 0 ) && ( null == accessor.Sparse ) ) {
        var bufferView = model.BufferViews[ accessor.BufferView.Value ];
        var buffer = model.LoadBinaryBuffer( bufferView.Buffer, modelFilepath );
        result = new PointCollection();
        int sourceOffset = accessor.ByteOffset + bufferView.ByteOffset;
        int stride = bufferView.ByteStride.HasValue ? bufferView.ByteStride.Value : 8;
        for ( int i = 0; i < accessor.Count; i++ ) {
          float x = BitConverter.ToSingle( buffer, sourceOffset );
          float y = BitConverter.ToSingle( buffer, sourceOffset + 4 );
          result.Add( new System.Windows.Point( x, y ) );
          sourceOffset += stride;
        }        
      }
      return result;
    }

    private Int32Collection? ReadScalar( Gltf model, Accessor accessor )
    {
      Int32Collection? result = null;
      // TODO support sparse accessors, and validate count, byte length, byte offset, etc.
      if ( ( Accessor.TypeEnum.SCALAR == accessor.Type ) && ( Accessor.ComponentTypeEnum.FLOAT != accessor.ComponentType ) && accessor.BufferView.HasValue && ( accessor.Count > 0 ) && ( null == accessor.Sparse ) ) {
        var bufferView = model.BufferViews[ accessor.BufferView.Value ];
        var buffer = model.LoadBinaryBuffer( bufferView.Buffer, modelFilepath );
        result = new Int32Collection();
        int sourceOffset = accessor.ByteOffset + bufferView.ByteOffset;

        switch ( accessor.ComponentType ) {
          case Accessor.ComponentTypeEnum.BYTE:
          case Accessor.ComponentTypeEnum.UNSIGNED_BYTE: {
            for ( int i = 0; i < accessor.Count; i++, sourceOffset++ ) {
              result.Add( buffer[ sourceOffset ] );
            }
            break;
          }
          case Accessor.ComponentTypeEnum.SHORT: {
            for ( int i = 0; i < accessor.Count; i++, sourceOffset += 2 ) {
              result.Add( BitConverter.ToInt16( buffer, sourceOffset ) );
            }
            break;
          }
          case Accessor.ComponentTypeEnum.UNSIGNED_SHORT: {
            for ( int i = 0; i < accessor.Count; i++, sourceOffset += 2 ) {
              result.Add( BitConverter.ToUInt16( buffer, sourceOffset ) );
            }
            break;
          }
          case Accessor.ComponentTypeEnum.UNSIGNED_INT: {
            for ( int i = 0; i < accessor.Count; i++, sourceOffset += 4 ) {
              result.Add( Convert.ToInt32( BitConverter.ToUInt32( buffer, sourceOffset ) ) );
            }
            break;
          }
        }
      }

      return result;
    }

    private DiffuseMaterial GetMaterial( Gltf model, MeshPrimitive primitive )
    {
      DiffuseMaterial material = new DiffuseMaterial();
      if ( primitive.Material.HasValue ) {
        var gltfMaterial = model.Materials[ primitive.Material.Value ];
        var baseTexture = gltfMaterial.PbrMetallicRoughness.BaseColorTexture;

        string? imageFilepath = null;

        if ( null != baseTexture ) {
          var textureInfo = model.Textures[ baseTexture.Index ];
          if ( ( null != textureInfo ) && textureInfo.Source.HasValue ) {
            var imageInfo = model.Images[ textureInfo.Source.Value ];
            if ( null != imageInfo ) {
              if ( String.IsNullOrEmpty( imageInfo.Uri ) ) {
                var imageStream = model.OpenImageFile( textureInfo.Source.Value, modelFilepath );
                imageFilepath = System.IO.Path.GetTempPath();
                Guid guid = Guid.NewGuid();
                imageFilepath += guid;
                var mimeType = imageInfo.MimeType.GetValueOrDefault( Image.MimeTypeEnum.image_jpeg );
                switch ( mimeType ) {
                  case Image.MimeTypeEnum.image_jpeg: {
                    imageFilepath += ".jpg";
                    break;
                  }
                  case Image.MimeTypeEnum.image_png: {
                    imageFilepath += ".png";
                    break;
                  }
                }
                var fileStream = System.IO.File.Create( imageFilepath );
                imageStream.CopyTo( fileStream );
                fileStream.Close();
                tempTextureFiles.Add( imageFilepath );
              } else {
                imageFilepath = imageInfo.Uri;
                if ( !System.IO.Path.IsPathFullyQualified( imageFilepath ) ) {
                  imageFilepath = System.IO.Path.GetDirectoryName( modelFilepath ) + imageFilepath;
                }
              }
            }
          }
        }
        
        if ( String.IsNullOrEmpty( imageFilepath ) || !System.IO.File.Exists( imageFilepath ) ) {
          float[]? baseColorFactor = gltfMaterial.PbrMetallicRoughness.BaseColorFactor;
          if ( ( baseColorFactor != null ) && ( baseColorFactor.Length >= 3 ) ) {
            // TODO linear to sRGB color space conversion
            Color color = Color.FromRgb( (byte)( baseColorFactor[0] * 255 ), (byte)( baseColorFactor[1] * 255 ), (byte)( baseColorFactor[2] * 255 ) );
            material.Brush = new SolidColorBrush(color);
          } else {
            material.Brush = new SolidColorBrush( System.Windows.Media.Colors.HotPink );
          }
        } else {
          var imageSource = new BitmapImage( new Uri( imageFilepath ) );
          material.Brush = new ImageBrush( imageSource );
        }

      }
      return material;
    }

    private string? modelFilepath = null;

    private List<string> tempTextureFiles = new List<string>();

    public List<Model3DGroup>? scenes { get; }
  }
}
