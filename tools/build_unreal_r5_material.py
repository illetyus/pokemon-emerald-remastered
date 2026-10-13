"""Editor-only builder for the source indexed metatile shader; no source art is imported."""
import hashlib
import json
import os
from pathlib import Path
import unreal

root=Path(__file__).resolve().parents[1]
evidence=Path(os.environ['AP04_R5_EVIDENCE']).resolve()
assert evidence.is_relative_to((root/'local').resolve())
evidence.mkdir(parents=True,exist_ok=True)
destination='/Game/Local/AP04/R5'
name='M_R5Indexed'
asset_path=destination+'/'+name
assert not unreal.EditorAssetLibrary.does_asset_exist(asset_path),'Fresh material creation required'
material=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,destination,unreal.Material,unreal.MaterialFactoryNew())
assert material
material.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('two_sided',True)
unreal.MaterialEditingLibrary.set_base_material_usage(material,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
library=unreal.MaterialEditingLibrary

def node(kind,x=0,y=0):
    result=library.create_material_expression(material,kind,x,y)
    assert result
    return result

def connect(first,second,input_name,output=''):
    assert library.connect_material_expressions(first,output,second,input_name),(first.get_name(),input_name)

inputs={}
inputs['UV']=node(unreal.MaterialExpressionTextureCoordinate,-1500,0)
for q in range(4):
    fields=[]
    for field in range(4):
        entry=node(unreal.MaterialExpressionPerInstanceCustomData,-1800,100+q*240+field*50)
        entry.set_editor_property('data_index',q*4+field)
        fields.append(entry)
    xy=node(unreal.MaterialExpressionAppendVector);zw=node(unreal.MaterialExpressionAppendVector)
    connect(fields[0],xy,'A');connect(fields[1],xy,'B')
    connect(fields[2],zw,'A');connect(fields[3],zw,'B')
    packed=node(unreal.MaterialExpressionAppendVector)
    connect(xy,packed,'A');connect(zw,packed,'B')
    interpolator=node(unreal.MaterialExpressionVertexInterpolator)
    connect(packed,interpolator,'')
    inputs['Q'+str(q)]=interpolator
default_texture=unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture')
assert default_texture
for pin,parameter in [('IndexTex','R5_TileIndexTexture'),('PaletteTex','R5_PaletteTexture')]:
    entry=node(unreal.MaterialExpressionTextureObjectParameter)
    entry.set_editor_property('parameter_name',parameter)
    entry.set_editor_property('texture',default_texture)
    entry.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if default_texture.get_editor_property('srgb') else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    inputs[pin]=entry
for pin,parameter,value in [('Width','R5_TileSheetWidth',128),('Height','R5_TileSheetHeight',512),('Columns','R5_TilesPerRow',16),('Pixels','R5_SourceTilePixels',8)]:
    entry=node(unreal.MaterialExpressionScalarParameter)
    entry.set_editor_property('parameter_name',parameter);entry.set_editor_property('default_value',float(value))
    inputs[pin]=entry
custom=node(unreal.MaterialExpressionCustom,-200,0)
custom.set_editor_property('description','Source indexed metatile VRAM lookup')
custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT4)
custom_inputs=[]
for pin in inputs:
    entry=unreal.CustomInput();entry.set_editor_property('input_name',pin);custom_inputs.append(entry)
custom.set_editor_property('inputs',custom_inputs)
code='''
float2 pixel = clamp(floor(UV * Pixels * 2.0), 0.0, Pixels * 2.0 - 1.0);
int2 quadrant = (int2)floor(pixel / Pixels);
int q = quadrant.x + quadrant.y * 2;
float4 tile = q == 0 ? Q0 : (q == 1 ? Q1 : (q == 2 ? Q2 : Q3));
float2 local = pixel - quadrant * Pixels;
if (tile.z > 0.5) local.x = Pixels - 1.0 - local.x;
if (tile.w > 0.5) local.y = Pixels - 1.0 - local.y;
float id = floor(tile.x + 0.5);
float2 tileOrigin = float2(fmod(id, Columns), floor(id / Columns)) * Pixels;
float2 atlasUV = (tileOrigin + local + 0.5) / float2(Width, Height);
float index = floor(Texture2DSample(IndexTex, IndexTexSampler, atlasUV).r * 255.0 + 0.5);
float palette = floor(tile.y + 0.5);
float4 color = Texture2DSample(PaletteTex, PaletteTexSampler, float2((index + 0.5) / 16.0, (palette + 0.5) / 16.0));
// The immutable RGB palette LUT is opaque; GBA transparency belongs to the pixel index.
color.a = index == 0.0 ? 0.0 : color.a;
return color;
'''
custom.set_editor_property('code',code)
for pin,entry in inputs.items():
    connect(entry,custom,pin)
rgb=node(unreal.MaterialExpressionComponentMask);rgb.set_editor_property('r',True);rgb.set_editor_property('g',True);rgb.set_editor_property('b',True);rgb.set_editor_property('a',False)
alpha=node(unreal.MaterialExpressionComponentMask);alpha.set_editor_property('r',False);alpha.set_editor_property('g',False);alpha.set_editor_property('b',False);alpha.set_editor_property('a',True)
connect(custom,rgb,'');connect(custom,alpha,'')
assert library.connect_material_property(rgb,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
assert library.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY_MASK)
library.recompile_material(material)
assert library.get_num_material_expressions(material)==42
assert set(map(str,library.get_texture_parameter_names(material)))=={'R5_TileIndexTexture','R5_PaletteTexture'}
assert set(map(str,library.get_scalar_parameter_names(material)))=={'R5_TileSheetWidth','R5_TileSheetHeight','R5_TilesPerRow','R5_SourceTilePixels'}
assert unreal.EditorAssetLibrary.save_loaded_asset(material,False)
assert unreal.EditorAssetLibrary.does_asset_exist(asset_path)
receipt={'Asset':material.get_path_name(),'BuilderSHA256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    'CustomShaderSHA256':hashlib.sha256(code.encode()).hexdigest(),'TextureParameters':['R5_TileIndexTexture','R5_PaletteTexture'],
    'ScalarParameters':['R5_TileSheetWidth','R5_TileSheetHeight','R5_TilesPerRow','R5_SourceTilePixels'],
    'PerInstanceIndices':list(range(16)),'VertexInterpolatorVectors':4,'SourceTilesPrimary':512,'SourcePalettesPrimary':6,
    'ActualEditorCreatedAndSaved':True,'PersistentReloadVerified':False,'GPUVisualVerified':False,'InputArtImported':False,
    'TransparentPaletteIndex':0,'PaletteLutBytesUnchanged':True,
    'IndexedTransparencyReference':'https://gbadev.net/tonc/objbg.html',
    'Documentation':'https://dev.epicgames.com/documentation/unreal-engine/custom-material-expressions-in-unreal-engine?lang=en-US'}
(evidence/'material-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
unreal.log('AP04_R5_INDEXED_MATERIAL_SAVED '+material.get_path_name())
