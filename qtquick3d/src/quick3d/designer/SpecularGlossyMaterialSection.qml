// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    Section {
        caption: qsTr("Specular Glossy Material")
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Alpha Mode")
                tooltip: qsTr("Sets the mode for how the alpha channel of material color is used.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "SpecularGlossyMaterial"
                    model: ["Default", "Mask", "Blend", "Opaque"]
                    backendValue: backendValues.alphaMode
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Alpha Cutoff")
                tooltip: qsTr("Specifies the cutoff value when using the Mask alphaMode.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.alphaCutoff
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Blend Mode")
                tooltip: qsTr("Determines how the colors of the model rendered blend with those behind it.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "SpecularGlossyMaterial"
                    model: ["SourceOver", "Screen", "Multiply"]
                    backendValue: backendValues.blendMode
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Lighting")
                tooltip: qsTr("Defines which lighting method is used when generating this material.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "SpecularGlossyMaterial"
                    model: ["NoLighting", "FragmentLighting"]
                    backendValue: backendValues.lighting
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Point Size")
                tooltip: qsTr("This property determines the size of the points rendered, when the geometry is using a primitive type of points.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: -9999999
                    maximumValue: 9999999
                    decimals: 2
                    backendValue: backendValues.pointSize
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Line Width")
                tooltip: qsTr("This property determines the width of the lines rendered, when the geometry is using a primitive type of lines or line strips.")
            }
            SecondColumnLayout {
                SpinBox {
                    minimumValue: -9999999
                    maximumValue: 9999999
                    decimals: 2
                    backendValue: backendValues.lineWidth
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Albedo")
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Albedo Color")
            }

            ColorEditor {
                backendValue: backendValues.albedoColor
                supportGradient: false
            }

            PropertyLabel {
                text: qsTr("Albedo Map")
                tooltip: qsTr("Defines a texture used to set the base color of the material.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    backendValue: backendValues.albedoMap
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                    + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Specular")
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Specular Color")
            }

            ColorEditor {
                backendValue: backendValues.specularColor
                supportGradient: false
            }

            PropertyLabel {
                text: qsTr("Specular Map")
                tooltip: qsTr("Defines a texture used to set the specular color of the material.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    backendValue: backendValues.specularMap
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                    + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Glossiness")
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Glossiness")
                tooltip: qsTr("Controls the size of the specular highlight generated from lights, and the clarity of reflections in general.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.glossiness
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Glossiness Map")
                tooltip: qsTr("Defines a texture to control the glossiness of the material.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    backendValue: backendValues.glossinessMap
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Glossiness Channel")
                tooltip: qsTr("Defines the texture channel used to read the glossiness value from glossinessMap.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "Material"
                    model: ["R", "G", "B", "A"]
                    backendValue: backendValues.glossinessChannel
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Normal")
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Normal Map")
                tooltip: qsTr("Defines an RGB image used to simulate fine geometry displacement across the surface of the material.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    backendValue: backendValues.normalMap
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Normal Strength")
                tooltip: qsTr("Controls the amount of simulated displacement for the normalMap.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.normalStrength
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Occlusion")
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Occlusion Amount")
                tooltip: qsTr("Contains the factor used to modify the values from the occlusionMap texture.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.occlusionAmount
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Occlusion Map")
                tooltip: qsTr("Defines a texture used to determine how much indirect light the different areas of the material should receive.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    backendValue: backendValues.occlusionMap
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Occlusion Channel")
                tooltip: qsTr("Defines the texture channel used to read the occlusion value from occlusionMap.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "Material"
                    model: ["R", "G", "B", "A"]
                    backendValue: backendValues.occlusionChannel
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Opacity")
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Opacity")
                tooltip: qsTr("Drops the opacity of just this material, separate from the model.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.opacity
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Opacity Map")
                tooltip: qsTr("Defines a texture used to control the opacity differently for different parts of the material.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    backendValue: backendValues.opacityMap
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Opacity Channel")
                tooltip: qsTr("Defines the texture channel used to read the opacity value from opacityMap.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "Material"
                    model: ["R", "G", "B", "A"]
                    backendValue: backendValues.opacityChannel
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Height")
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Height Map")
                tooltip: qsTr("This property defines a texture used to determine the height the texture will be displaced when rendered through the use of Parallax Mapping.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    backendValue: backendValues.heightMap
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Height Channel")
                tooltip: qsTr("This property defines the texture channel used to read the height value from heightMap.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "Material"
                    model: ["R", "G", "B", "A"]
                    backendValue: backendValues.heightChannel
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Height Amount")
                tooltip: qsTr("This property contains the factor used to modify the values from the heightMap texture.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1
                    decimals: 2
                    backendValue: backendValues.heightAmount
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Min Height Map Samples")
                tooltip: qsTr("This property defines the minimum number of samples used for performing Parallex Occlusion Mapping using the heightMap.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 9999999
                    decimals: 0
                    backendValue: backendValues.minHeightMapSamples
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Max Height Map Samples")
                tooltip: qsTr("This property defines the maximum number of samples used for performing Parallex Occlusion Mapping using the heightMap.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 9999999
                    decimals: 0
                    backendValue: backendValues.maxHeightMapSamples
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Clearcoat")
        width: parent.width

        SectionLayout {

            PropertyLabel {
                text: qsTr("Clearcoat Amount")
                tooltip: qsTr("This property contains the intensity of the clearcoat layer.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1
                    decimals: 2
                    backendValue: backendValues.clearcoatAmount
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Clearcoat Map")
                tooltip: qsTr("This property defines a texture used to determine the intensity of the clearcoat layer.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    backendValue: backendValues.clearcoatMap
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Clearcoat Channel")
                tooltip: qsTr("This property defines the texture channel used to read the intensity from clearcoatMap.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "Material"
                    model: ["R", "G", "B", "A"]
                    backendValue: backendValues.clearcoatChannel
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Clearcoat Roughness Amount")
                tooltip: qsTr("This property defines the roughness of the clearcoat layer.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1
                    decimals: 2
                    backendValue: backendValues.clearcoatRoughnessAmount
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Clearcoat Roughness Map")
                tooltip: qsTr("This property defines a texture used to determine the roughness of the clearcoat layer.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    backendValue: backendValues.clearcoatRoughnessMap
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Clearcoat Roughness Channel")
                tooltip: qsTr("This property defines the texture channel used to read the roughness from clearcoatRoughnessMap.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "Material"
                    model: ["R", "G", "B", "A"]
                    backendValue: backendValues.clearcoatChannel
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Clearcoat Normal Map")
                tooltip: qsTr("This property defines a texture used as a normalMap for the clearcoat layer.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    backendValue: backendValues.clearcoatNormalMap
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Transmission")
        width: parent.width

        SectionLayout {

            PropertyLabel {
                text: qsTr("Transmission Factor")
                tooltip: qsTr("This property contains the base percentage of light that is transmitted through the surface.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1
                    decimals: 2
                    backendValue: backendValues.transmissionFactor
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Transmission Map")
                tooltip: qsTr("This property defines a texture that contains the transmission percentage of a the surface.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    backendValue: backendValues.transmissionMap
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Transmission Channel")
                tooltip: qsTr("This property defines the texture channel used to read the transmission percentage from transmissionMap.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "Material"
                    model: ["R", "G", "B", "A"]
                    backendValue: backendValues.transmissionChannel
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Emissive Color")
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Emissive Map")
                tooltip: qsTr("Sets a texture to be used to set the emissive factor for different parts of the material.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    backendValue: backendValues.emissiveMap
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                    + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Emissive Factor")
                tooltip: qsTr("This property determines the color of self-illumination for this material.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                    + StudioTheme.Values.actionIndicatorWidth
                    minimumValue: -9999999
                    maximumValue: 9999999
                    decimals: 2
                    backendValue: backendValues.emissiveFactor_x
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    text: "X"
                    color: StudioTheme.Values.theme3DAxisXColor
                }

                ExpandingSpacer {}
            }

            PropertyLabel {}

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                    + StudioTheme.Values.actionIndicatorWidth
                    minimumValue: -9999999
                    maximumValue: 9999999
                    decimals: 2
                    backendValue: backendValues.emissiveFactor_y
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    text: "Y"
                    color: StudioTheme.Values.theme3DAxisYColor
                }

                ExpandingSpacer {}
            }

            PropertyLabel {}

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                    + StudioTheme.Values.actionIndicatorWidth
                    minimumValue: -9999999
                    maximumValue: 9999999
                    decimals: 2
                    backendValue: backendValues.emissiveFactor_z
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    text: "Z"
                    color: StudioTheme.Values.theme3DAxisZColor
                }

                ExpandingSpacer {}
            }
        }
    }
}
