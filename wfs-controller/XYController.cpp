//
// Created by tar on 10/11/22.
//

#include "XYController.h"


XYController::XYController(uint maxNumNodes) : maxNodes(maxNumNodes) {}

void XYController::paint(Graphics &g) {
    Component::paint(g);
    g.fillAll(Colours::whitesmoke);

    g.setColour(juce::Colours::lightgrey);
    g.drawRect(getLocalBounds(), 1);

    // Nodes are automatically painted, as they are added as child components.
}

void XYController::resized() {
    for (auto &node: nodes) {
        node.second->setBounds();
    }
}

void XYController::mouseDown(const MouseEvent &event) {
    if (event.mods.isPopupMenu()) {
        // Maybe add a node
        PopupMenu m;
        if (event.originalComponent == this) {
            if (canAddNode()) { m.addItem(1, "Create node"); }
            if (!nodes.empty()) { m.addItem(2, "Remove all nodes"); }
            m.showMenuAsync(PopupMenu::Options(), [this, event](int result) {
                if (result == 1) {
                    // Find position centered on the location of the click.
                    createNode(event.position);
                } else if (result == 2) {
                    removeAllNodes();
                }
            });
        }
    }
}

void XYController::createNode(Point<float> position) {
    // Normalise the position to get the value.
    auto bounds{getBounds().toFloat()};
    Node::Value value{position.x / bounds.getWidth(), 1 - position.y / bounds.getHeight()};

    auto key{getNextAvailableNodeID()};
    DBG("XYController: Adding node with ID " << String(key));
    nodes[key] = std::make_unique<Node>(value, key);
    auto *node{nodes[key].get()};
    addAndMakeVisible(node);
    node->setBounds();

    node->onMove = [this](Node *nodeBeingMoved) {
        // Set node bounds here
        nodeBeingMoved->setBounds();
        // And do value change callback
        if (onValueChange != nullptr) {
            // This isn't nice.
            for (auto it = nodes.begin(); it != nodes.end(); ++it) {
                if (it->second.get() == nodeBeingMoved) {
                    onValueChange(it->first, {nodeBeingMoved->value.x, nodeBeingMoved->value.y});
                    return;
                }
            }
        }
    };

    node->onRemove = [this](Node *nodeToRemove) {
        removeNode(nodeToRemove);
    };

    if (onValueChange != nullptr) {
        onValueChange(key, {node->value.x, node->value.y});
    }

    if (onAddNode != nullptr) {
        onAddNode(key);
    }

    repaint(node->getBounds());
}

void XYController::normalisePosition(Point<float> &position) {
    position.x /= static_cast<float>(getWidth());
    position.y = 1 - position.y / static_cast<float>(getHeight());
}

void XYController::removeNode(Node *const node) {
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        if (it->second.get() == node) {
            removeNodeByIterator(it);
            return;
        }
    }
}

void XYController::removeAllNodes() {
    auto it{nodes.begin()};
    while (it != nodes.end()) {
        it = removeNodeByIterator(it);
    }
}

std::unordered_map<uint, std::unique_ptr<XYController::Node>>::iterator
XYController::removeNodeByIterator(std::unordered_map<uint, std::unique_ptr<XYController::Node>>::iterator it) {
    auto index{it->first};
    it = nodes.erase(it);
    if (onRemoveNode != nullptr) {
        DBG("XYController: Removing node with ID " << String(index));
        onRemoveNode(index);
    }
    return it;
}

uint XYController::getNextAvailableNodeID() {
    uint nextID{0};
    while (nodes.find(nextID) != nodes.end()) { nextID++; }
    return nextID;
}

void XYController::removeNode(uint index) {
    nodes.erase(index);
}

bool XYController::canAddNode() {
    return maxNodes == 0 || nodes.size() < maxNodes;
}

XYController::Node::Node(Value val, uint idx) : index(idx), value(val) {}

void XYController::Node::paint(Graphics &g) {
    auto colour{
            juce::Colours::steelblue.withRotatedHue(static_cast<float>(index) * 1 / juce::MathConstants<float>::twoPi)};
    g.setColour(colour);
    g.fillEllipse(getLocalBounds().toFloat());
    g.setColour(colour.darker(.25));
    g.drawEllipse(getLocalBounds().withSizeKeepingCentre(getWidth() - 2, getHeight() - 2).toFloat(), 2.f);
    g.setColour(Colours::white);
    g.setFont(20);
    g.drawText(String(index + 1), getLocalBounds(), Justification::centred);
}

void XYController::Node::mouseDown(const MouseEvent &event) {
    if (event.mods.isPopupMenu()) {
        // Try to remove this node.
        PopupMenu m;
        m.addItem(1, "Remove node");
        // TODO: expose possibility of adding more menu items via a callback.
        m.showMenuAsync(PopupMenu::Options(), [this, event](int result) {
            if (result == 1 && onRemove != nullptr) {
                onRemove(this);
            }
        });
    }
}

void XYController::Node::mouseDrag(const MouseEvent &event) {
    if (event.mods.isLeftButtonDown()) {
        auto parent{getParentComponent()};
        auto parentBounds{parent->getBounds().toFloat()};

        auto parentBottomRight{Point<float>{parentBounds.getWidth(), parentBounds.getHeight()}};

        auto newVal{event.getEventRelativeTo(parent).position / parentBottomRight};

        // Set node value.
        value.x = clamp(newVal.x, 0., 1.);
        value.y = clamp(1 - newVal.y, 0., 1.);
        if (onMove != nullptr) {
            onMove(this);
        }
    }
}

float XYController::Node::clamp(float val, float min, float max) {
    if (val >= max) {
        val = max;
    } else if (val <= min) {
        val = min;
    }
    return val;
}

void XYController::Node::setBounds() {
    auto bounds{getParentComponent()->getBounds()};
    Component::setBounds(bounds.getWidth() * value.x - NODE_WIDTH / 2,
                         (bounds.getHeight() - bounds.getHeight() * value.y) - NODE_WIDTH / 2,
                         Node::NODE_WIDTH,
                         Node::NODE_WIDTH);
}