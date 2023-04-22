//Riley Taylor and Eli Dayney
//CS 302
//Dr. Scott Emerich
//Checks to see if a word can be spelled using dice combination of letters
//We use the Edmond's Karp algorithm to perform bipartite matching, and 
//find if the net flow to the sink is equal to the number of letters in the word,
//we can spell it.
//
//Uses network flow
#include <queue>
#include <typeinfo>
#include <fstream>
#include <iostream>
#include <vector>
#include <climits>

using namespace std;

typedef enum{SOURCE, DICE, WORD, SINK} Node_Type;

//edge cannot access node information directly
class Edge{
  
	public:
		//from -> to
		class Node *to; //tip
		class Node *from; //tail
		
		//constructor for edges
		Edge(class Node *to, class Node *from, bool reverse_edge) {
			this->to = to;
			this->from = from;
			this->reverse_edge = reverse_edge;

			if (!reverse_edge) {
				this->original = 1;
				this->residual = 0;
			}
			else {
				this->original = 0;
				this->residual = 1;
			}	
		
			if (!reverse_edge) {
				Edge *thisRev = new Edge(from, to, true);
				thisRev->reverse = this;
				this->reverse = thisRev;
			}
		
		}; 
    
		~Edge() {
		};

		void flipWeights() {
			//if (this->original == 1) this->original == 0;
			//else if (this->original == 
		}

		bool reverse_edge; //is a reverse?
		Edge *reverse; //pointer to rev edge
		int original; //org weight/edge
		int residual; //allows updated weighting during Edmonds-Karp
};

class Node{

	public:	
		//statically allocated members
		string word; //node string
        int id;
        Node_Type type; //source, sink, word or dice
        vector<bool> letters; //check if word contains (i + 1)th letter in alphabet
        int visited; //for BFS
		
		string retEnumVal(Node_Type type) {
			switch (type) {
				case SINK:
					return "SINK";
				case DICE:
					return "DICE";
				case SOURCE:
					return "SOURCE";
				case WORD:
					return "WORD";
			}
			
			return ""; //likely redundant
		}
		
		//constructor for nodes
		Node(int id, Node_Type type, string word) {
			this->id = id;
			this->type = type;
			//cout<<nameof(this->type)<<endl;
			this->word = word;
			this->visited = false;

			//initialize indices of letters
			for (int i = 0; i < 26; i++) {
				if (word.find(i + 'A') != string::npos) {
					letters.push_back(true);
				}
				else letters.push_back(false);
			}
		};
		
		//default destructor
		~Node() {
			for (int i = 0; i < adj.size(); i++) {
				delete adj[i];
			}
		};
	
		//tells if the node has this letter 
		bool has_letter(char c){
			return letters[c - 'A'];
		};
	
	    void dump_node() {
			cout<<"Node "<<this->id<<": "<<this->word<<" Edges to ";
			for (int i = 0; i < adj.size(); i++) {
				cout<<this->adj[i]->to->id<<" ";
			}
			cout<<endl; //<<"    Backedge from: "<<this->backedge->from->id<<endl;

			//cout<<"w/ backedge from "<<this->backedge->from->id<<endl;
		}
		
	    //dynamically allocated memebers
		vector<Edge*> adj; //adjacency list, pointers to outgoing edges
		Edge *backedge; //previous edge for Edmonds-Karp
};

class Graph {
	public:
		
		//constructor initializes graph with source node
		Graph() {
			this->source = new Node(0, SOURCE, "SOURCE");
			nodes.push_back(source);
			this->min_nodes = 1;
		};

		//destructor to deallocate memory of graph
		~Graph() {
			for (int i = 0; i < nodes.size(); i++) {
				delete nodes[i];
			}
		};

		Node *source; //not necessary but makes code more readable
		Node *sink;
		
		vector<Node *> nodes; //holds nodes
		vector<Node *> diceVec;
		vector<Node *> letterVec;
		vector<int> spellingIds; //order of dice to spell
		int min_nodes; //dice nodes + source node, newsize for new word 
		string word;

		//add dice to graph
		//also connects to source
		void add_dice_to_graph(string die, int id) {
			
			Node *newDie = new Node(id, DICE, die);
			Edge *toDie = new Edge(newDie, this->source, false); //(to, from, reverse)
			newDie->adj.push_back(toDie->reverse);
			this->source->adj.push_back(toDie);
			nodes.push_back(newDie);
			diceVec.push_back(newDie);
			min_nodes++;
			
		};

		//error checking
		void dump_graph() {
			for (int i = 0; i < nodes.size(); i++) {
				nodes[i]->dump_node();

				for (int j = 0; j< nodes[i]->adj.size(); j++) {
					Edge *edges= nodes[i]->adj[j];
					cout << "Edge " << i << ": residual=" << edges->residual << ", original=" << edges->original << endl;

				}
			}

			//cout<<min_nodes<<endl;
		}

		//add word (letter) nodes to graph
		//also connects to sink
		void add_word_to_graph(string word, int id) {
			Node *letter = new Node(id, WORD, word);
			this->nodes.push_back(letter);
			//if we're adding a sink, we attach all the words to it
			if (word == "SINK") {
				letter->type = SINK;
				this->sink = letter;
				for (int i = 0; i < letterVec.size(); i++) {
					Edge *toSink = new Edge(letter, letterVec[i], false); //to, from, isReverse
					letterVec[i]->adj.push_back(toSink);
					letter->adj.push_back(toSink->reverse);
				}
				
            }
			//else we attach all associated dice to the letter node we're adding
			else {
				//loops through dice, if they contain the letter in letter node add an edge
	            for (int i = 0; i < diceVec.size(); i++) {
		            if ((this->diceVec[i]->word).find(word) != string::npos) {
			            Edge *toWord = new Edge(letter, this->diceVec[i], false);
				        this->diceVec[i]->adj.push_back(toWord);
					    letter->adj.push_back(toWord->reverse);
					}
				}

				letterVec.push_back(letter);
			}

		};

		//breadth first search for Edmonds-Karp
		bool BFS() {
			queue<Node *> q;
			//reset visited and backedges
			for (auto n : nodes) {
				n->visited = 0;
				n->backedge = nullptr;
			}
			
			source->visited = 1;
			q.push(source);
			
			//push source to queue, pop front of queue and add unvisited neighbors w/ net flow
			while (!q.empty()) {
				Node *curr = q.front();
				q.pop();
				for (auto e : curr->adj) {
					if (e->original == 1 && e->to->visited == 0) {
						e->to->visited = 1;
						e->to->backedge = e;
						if (e->to->word == "SINK") { //if it reaches sink, a path is found
							return true;
						}
						q.push(e->to);
					}
				}
			}

			return false;
		};
		
		//checks if word can be spelled, and adds dice nodes order to spellingIds
		bool canISpell() {
			while (BFS()) {
				Node *cur = sink;
				while (cur != source) { //loops from sink to source
					Edge * e = cur->backedge;
					//switches residual and original for foward and reverse edges
					e->residual = 1;
					e->original = 0;

					e->reverse->residual = 0;
					e->reverse->original = 1;

					cur = e->from;
				}
			}

			//loops through word and adds order of dice used
			for (int i = min_nodes; i < nodes.size()-1; i++) {
				for (int j = 0; j < nodes[i]->adj.size()-1; j++) {
					Edge *e = nodes[i]->adj[j];
					if (e->to->type == DICE && e->original == 1) {
						spellingIds.push_back(nodes[i]->adj[j]->to->id-1);
					}
				}
			}

			//if the number of dice used is the same as the word, then it can be spelled
			if (spellingIds.size() == word.length()) return true;
			return false;

		}
		
		//print spelling Ids and word
		void print_node_order(string word) {
			int num = spellingIds.size();
			for (int i = 0; i < num - 1; i++) {
				cout<<spellingIds[i] << ",";
			}

			cout<<spellingIds[num - 1] << ": " << word <<endl;
		
		};
};

int main(int argc, char *argv[]) {
    //argc is the number of command-line arguments
    //argv is an array of strings containing the command-line arguments
    if (argc != 3) {
        cout << "Usage: ./program dice_file word_file" << endl;
        return 1;
    }

    //open dice & word files
    ifstream dice(argv[1]);
    ifstream word(argv[2]);

    //check if files are successfully opened
    if (!dice || !word) {
        cout << "Error opening files" << endl;
        return 1;
    }

	vector<string> dies;
	string die;
	while (getline(dice, die)) { //adds dice to vector
		dies.push_back(die);
	}

	string line;
	while (getline(word, line)) {
		Graph g;
		int i = 1;
		string die;
		for (int j = 0; j < dies.size(); j++) { //adds dice to graph
			g.add_dice_to_graph(dies[j], i);
			i++;
		}

		for (int q = 0; q < line.length(); q++) {
			g.add_word_to_graph(string(1, line.at(q)), i); //adds word to graph
			i++;
		}

		g.word = line;

		g.add_word_to_graph("SINK", i); //adds sink
			
		//if word can be spelled, print it, else print it cant
		if (g.canISpell()) {
			g.print_node_order(g.word);
		}
		else cout << "Cannot spell " << line<< endl;

		g.spellingIds.clear();
	}

    // Close files
    dice.close();
    word.close();

    return 0;
}

