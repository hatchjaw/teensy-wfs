//
// Created by tar on 10/11/22.
//

#ifndef JACKTRIP_TEENSY_XYCONTROLLER_H
#define JACKTRIP_TEENSY_XYCONTROLLER_H

#include <JuceHeader.h>

class XYController : public Component {
public:
    explicit XYController(uint maxNumNodes = 0);

    void paint(Graphics &g) override;

    void mouseDown(const MouseEvent &event) override;

    void resized() override;

    /**
     * Exposing this publicly is probably a bad idea, but it provides an easy
     * way of rolling back the creation of a node if some important condition
     * isn't satisfied.
     * @param index
     */
    void removeNode(uint index);

    std::function<void(uint nodeIndex, Point<float>)> onValueChange;

    std::function<void(uint nodeIndex)> onAddNode;

    std::function<void(uint nodeIndex)> onRemoveNode;

protected:
    class Node : public Component {
    private:
        struct Value {
            float x, y;
        };
    public:
        explicit Node(Value val, uint idx = 0);

        void paint(Graphics &g) override;

        void mouseDown(const MouseEvent &event) override;

        void mouseDrag(const MouseEvent &event) override;

        std::function<void(Node *)> onMove;

        std::function<void(Node *)> onRemove;
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Node)

        void setBounds();

        static constexpr float NODE_WIDTH{50.f};

        uint index{0};
        Value value{};

        friend class XYController;

        static float clamp(float val, float min, float max);
    };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XYController)

    std::unordered_map<uint, std::unique_ptr<Node>> nodes;

    void createNode(Point<float> value);

    void normalisePosition(Point<float> &position);

    void removeNode(Node *node);

    uint getNextAvailableNodeID();

    void removeAllNodes();

    std::unordered_map<uint, std::unique_ptr<XYController::Node>>::iterator
    removeNodeByIterator(std::unordered_map<uint, std::unique_ptr<XYController::Node>>::iterator it);

    uint maxNodes;

    bool canAddNode();
};


#endif //JACKTRIP_TEENSY_XYCONTROLLER_H
