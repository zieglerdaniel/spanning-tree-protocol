#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>

#define MINIMUM_NUMBER_OF_MESSAGES 6
#define MAX_NODE_NAME_LENGTH 11
#define PATH_TO_GRAPH_FILE "../graph.txt"

class Edge;
class Node;

class Message {
public:
	Message(Node *src, Edge *usedEdge, int rootId, int costSumToRoot): src(src), usedEdge(usedEdge), rootId(rootId),
	                                                                   costSumToRoot(costSumToRoot) {
	}

	Node *src;
	Edge *usedEdge;
	int rootId;
	int costSumToRoot;
};

class Edge {
public:
	Edge(Node *nodeA, Node *nodeB, int costs): nodeA(nodeA), nodeB(nodeB), costs(costs) {
	}

	Node *nodeA;
	Node *nodeB;
	int costs;
};

class Node {
public:
	Node(char *name, int id): name(name), id(id) {
	}

	char *name;
	int id;

	int edgeCount = 0;
	Edge **connectedEdges = nullptr;

	std::vector<Message *> inbox;

	int rootId = id;
	int costSumToRoot = 0;
	Edge *nextHopToRoot = nullptr;

	bool hasConnection(Node *nodeB) {
		for (int i = 0; i < edgeCount; i++) {
			if (connectedEdges[i]->nodeB == nodeB) {
				return true;
			}
		}
		return false;
	}

	void sendMessage(Node *dest, Message *message) {
		dest->inbox.push_back(message);
	}

	void broadcastMessage(int withoutNodeId) {
		for (int i = 0; i < edgeCount; i++) {
			Edge *nextEdge = connectedEdges[i];
			if (nextEdge->nodeB == this || withoutNodeId == nextEdge->nodeB->id) {
				continue;
			}

			Node *nextNode = nextEdge->nodeB;
			Message *message = new Message(this, nextEdge, rootId, costSumToRoot);

			sendMessage(nextNode, message);
		}
	}

	void checkReceivedMessage() {
		if (inbox.empty()) {
			return;
		}

		bool updateNeeded = false;
		int bestSenderId = -1;

		for (Message *msg: inbox) {
			if (rootId > msg->rootId) {
				rootId = msg->rootId;
				costSumToRoot = msg->costSumToRoot + msg->usedEdge->costs;
				nextHopToRoot = msg->usedEdge;
				updateNeeded = true;
				bestSenderId = msg->src->id;
			} else if (rootId == msg->rootId) {
				int newCost = msg->costSumToRoot + msg->usedEdge->costs;
				if (newCost < costSumToRoot) {
					costSumToRoot = newCost;
					nextHopToRoot = msg->usedEdge;
					updateNeeded = true;
					bestSenderId = msg->src->id;
				}
			}
			delete msg;
		}

		inbox.clear();

		if (updateNeeded) {
			broadcastMessage(bestSenderId);
		}
	}
};

Node **nodes = nullptr;
int nodeIndex = 0;

void addBiConnection(Node *nodeA, Node *nodeB, int costs);

void addConnection(Node *nodeA, Node *nodeB, int costs);

Node *getNodeByName(char *name);

Node *getNodeById(int id);

void endAlgorithm();

void readGraphFile() {
	FILE *file = fopen(PATH_TO_GRAPH_FILE, "r");
	if (!file) {
		perror("Could not open graph file");
		exit(1);
	}

	char line[256];
	while (fgets(line, 255, file)) {
		if (line[0] == '\n' || line[0] == '/') continue;

		char *token1 = strtok(line, " \t\n;");
		if (!token1 || strcmp(token1, "Graph") == 0 || strcmp(token1, "}") == 0) continue;

		char *token2 = strtok(NULL, " \t\n;");
		char *token3 = strtok(NULL, " \t\n;");

		if (token2 && strcmp(token2, "=") == 0 && token3) {
			char *name = strdup(token1);
			int id = atoi(token3);
			nodes = (Node **) realloc(nodes, sizeof(Node *) * (nodeIndex + 1));
			nodes[nodeIndex++] = new Node(name, id);
			std::cout << "Node created: " << name << " (Id: " << id << ")\n";
		} else if (token2 && strcmp(token2, "-") == 0 && token3) {
			char *token4 = strtok(NULL, " \t\n;");
			char *token5 = strtok(NULL, " \t\n;");
			if (token5) {
				int cost = atoi(token5);
				addBiConnection(getNodeByName(token1), getNodeByName(token3), cost);
				std::cout << "Link: " << token1 << " <-> " << token3 << " Costs: " << cost << "\n";
			}
		}
	}
	fclose(file);
}

int main() {
	readGraphFile();
	std::cout << "--- Graph Loaded (" << nodeIndex << " Nodes) ---\n";

	for (int i = 0; i < nodeIndex; i++) {
		nodes[i]->broadcastMessage(-1);
	}

	int round = 1;
	while (true) {
		for (int k = 0; k < nodeIndex; k++) {
			nodes[k]->checkReceivedMessage();
		}

		if (round >= MINIMUM_NUMBER_OF_MESSAGES) {
			endAlgorithm();
			break;
		}
		round++;
	}
	return 0;
}

void endAlgorithm() {
	std::cout << "\n--- Spanning Tree found: ---\n";
	for (int i = 0; i < nodeIndex; i++) {
		Node *node = nodes[i];
		Node *root = getNodeById(node->rootId);

		const char *nextHopName = "-";
		int nextHopCosts = 0;
		if (node->nextHopToRoot != nullptr) {
			nextHopCosts = node->nextHopToRoot->costs;
			if (node->nextHopToRoot->nodeA == node) {
				nextHopName = node->nextHopToRoot->nodeB->name;
			} else {
				nextHopName = node->nextHopToRoot->nodeA->name;
			}
		}

		std::cout << "Node: " << node->name
				<< "\t (Id: " << node->id << ")"
				<< "\t Root: " << (root ? root->name : "-")
				<< "\t CostsToRoot: " << (node->costSumToRoot == 0 ? " 0" : std::to_string(node->costSumToRoot))
				<< "\t NextHop: " << nextHopName
				<< "\t NextHop Costs: " << nextHopCosts << std::endl;
	}
}

void addBiConnection(Node *nodeA, Node *nodeB, int costs) {
	if (!nodeA || !nodeB) return;
	addConnection(nodeA, nodeB, costs);
	addConnection(nodeB, nodeA, costs);
}

void addConnection(Node *nodeA, Node *nodeB, int costs) {
	if (nodeA->hasConnection(nodeB)) return;

	Edge *edge = new Edge(nodeA, nodeB, costs);
	nodeA->edgeCount++;
	nodeA->connectedEdges = (Edge **) realloc(nodeA->connectedEdges, nodeA->edgeCount * sizeof(Edge *));
	nodeA->connectedEdges[nodeA->edgeCount - 1] = edge;
}

Node *getNodeById(int id) {
	for (int i = 0; i < nodeIndex; i++) {
		if (nodes[i]->id == id) return nodes[i];
	}
	return nullptr;
}

Node *getNodeByName(char *name) {
	for (int i = 0; i < nodeIndex; i++) {
		if (strcmp(nodes[i]->name, name) == 0) return nodes[i];
	}
	return nullptr;
}
