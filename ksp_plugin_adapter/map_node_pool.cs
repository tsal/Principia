﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace principia {
namespace ksp_plugin_adapter {

internal class MapNodePool {

  public enum NodeSource {
    PREDICTION,
    FLIGHT_PLAN,
  }

  public MapNodePool() {
    nodes_ = new List<KSP.UI.Screens.Mapview.MapNode>();
    properties_ =
        new Dictionary<KSP.UI.Screens.Mapview.MapNode, MapNodeProperties>();
  }

  public void Clear() {
    foreach (var node in nodes_) {
      node.Terminate();
    }
    nodes_ = new List<KSP.UI.Screens.Mapview.MapNode>();
    properties_ =
        new Dictionary<KSP.UI.Screens.Mapview.MapNode, MapNodeProperties>();
    pool_index_ = 0;
  }

  public void Update() {
    for (int i = pool_index_; i < nodes_.Count; ++i) {
      nodes_[i].Terminate();
      properties_.Remove(nodes_[i]);
    }
    nodes_.RemoveRange(index : pool_index_, count : nodes_.Count - pool_index_);
    foreach (var node in nodes_) {
      node.NodeUpdate();
    }
    pool_index_ = 0;
  }

  public void RenderAndDeleteApsides(IntPtr apsis_iterator,
                                     CelestialBody celestial,
                                     MapObject.ObjectType type,
                                     NodeSource source) {
    for (; !apsis_iterator.IteratorAtEnd();
         apsis_iterator.IteratorIncrement()) {
      Vector3d apsis = (Vector3d)apsis_iterator.IteratorGetXYZ();
      MapNodeProperties node_properties;
      node_properties.object_type = type;
      node_properties.celestial = celestial;
      node_properties.world_position = apsis;
      node_properties.source = source;
      node_properties.time = apsis_iterator.IteratorGetTime();

      if (pool_index_ == nodes_.Count) {
        AddMapNodeToPool();
      }
      properties_[nodes_[pool_index_++]] = node_properties;
    }
    Interface.IteratorDelete(ref apsis_iterator);
  }

  private void AddMapNodeToPool() {
    var new_node = KSP.UI.Screens.Mapview.MapNode.Create(
        "apsis",
        // If we see this colour, something has gone wrong.
        XKCDColors.Pale,
        pixelSize : 32,
        hoverable : true,
        pinnable : true,
        blocksInput : true);
    new_node.OnUpdateVisible +=
        (KSP.UI.Screens.Mapview.MapNode node,
         KSP.UI.Screens.Mapview.MapNode.IconData icon) => {
          CelestialBody celestial = properties_[node].celestial;
          UnityEngine.Color colour =
              celestial.orbit == null
                  ? XKCDColors.SunshineYellow
                  : celestial.orbitDriver.Renderer.nodeColor;
          colour.a = 1;
          icon.visible = true;
          if (properties_[node].celestial.GetAltitude(
                  properties_[node].world_position) < 0) {
            // Make sure we see impacts.
            colour = XKCDColors.Orange;
          }
          icon.color = colour;
        };
    new_node.OnUpdateType +=
        (KSP.UI.Screens.Mapview.MapNode node,
         KSP.UI.Screens.Mapview.MapNode.TypeData type) => {
          if (properties_[node].celestial.GetAltitude(
                  properties_[node].world_position) < 0) {
            type.oType = MapObject.ObjectType.PatchTransition;
            type.pType =
                KSP.UI.Screens.Mapview.MapNode.PatchTransitionNodeType.Impact;
          } else {
            type.oType = properties_[node].object_type;
          }
        };
    new_node.OnUpdateCaption +=
        (KSP.UI.Screens.Mapview.MapNode node,
         KSP.UI.Screens.Mapview.MapNode.CaptionData caption) => {
          String name =
              properties_[node].object_type == MapObject.ObjectType.Periapsis
                  ? "Periapsis"
                  : "Apoapsis";
          caption.Header =
              properties_[node].celestial.name + " " + name + " : <color=" +
              XKCDColors.HexFormat.Chartreuse + ">" +
              properties_[node].celestial.GetAltitude(
                  properties_[node].world_position).ToString("N0",
                                                             Culture.culture) +
              " m</color>";
          caption.captionLine1 =
              "T" +
              FlightPlanner.FormatTimeSpan(TimeSpan.FromSeconds(
                  Planetarium.GetUniversalTime() - properties_[node].time));
          if (properties_[node].celestial.GetAltitude(
                  properties_[node].world_position) < 0) {
            caption.Header = properties_[node].celestial.name +
                             " Impact<color=" +
                             XKCDColors.HexFormat.Chartreuse + "></color>";
            caption.captionLine1 = "";
          }
          switch (properties_[node].source) {
            case NodeSource.FLIGHT_PLAN:
              caption.Header = "Planned " + caption.Header;
              break;
            case NodeSource.PREDICTION:
              caption.Header = "Predicted " + caption.Header;
              break;
            default:
              throw Log.Fatal("Unexpected node source " +
                              properties_[node].source);
          }
        };
    new_node.OnUpdatePosition +=
        (KSP.UI.Screens.Mapview.MapNode node) =>
        ScaledSpace.LocalToScaledSpace(
            properties_[node].world_position);
    nodes_.Add(new_node);
  }

  private struct MapNodeProperties {
    public MapObject.ObjectType object_type;
    public Vector3d world_position;
    public CelestialBody celestial;
    public NodeSource source;
    public double time;
  }

  private List<KSP.UI.Screens.Mapview.MapNode> nodes_;
  private Dictionary<KSP.UI.Screens.Mapview.MapNode,
                     MapNodeProperties> properties_;
  private int pool_index_ = 0;
}

}  // namespace ksp_plugin_adapter
}  // namespace principia
