#include "uno.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <map>
using namespace std;

enum Color { RED, GREEN, BLUE, YELLOW };
enum Type { NUMBER, SKIP, REVERSE, DRAW_TWO };

struct InternalCard {
    Color color;
    Type type;
    int value; // 0-9 for number cards, -1 for action ones

    string getColorString() const {
        switch (color) {
            case RED: return "Red";
            case GREEN: return "Green";
            case BLUE: return "Blue";
            case YELLOW: return "Yellow";
        }
        return "";
    }

    string getValueString() const {
        if (type == NUMBER) return to_string(value);
        if (type == SKIP) return "Skip";
        if (type == REVERSE) return "Reverse";
        if (type == DRAW_TWO) return "Draw Two";
        return "";
    }
};

struct GameState {
    int numPlayers;
    vector<vector<InternalCard>> playerHands;
    vector<InternalCard> deck;
    vector<InternalCard> discardPile;
    int currentPlayer;
    bool clockwise;
    bool gameOver;
    int winner;
};

static map<const UNOGame*, GameState> gameStates;

// make deck for all colors and cards
static void createDeck(GameState& state) {
    state.deck.clear();
    state.deck.reserve(76);

    for (int c = 0; c < 4; c++) {
        Color color = static_cast<Color>(c);

        // one zero card
        state.deck.push_back({color, NUMBER, 0});

        // two of each 1–9
        for (int v = 1; v <= 9; v++) {
            state.deck.push_back({color, NUMBER, v});
            state.deck.push_back({color, NUMBER, v});
        }

        // two skip, reverse, draw two for each color
        for (int i = 0; i < 2; i++) {
            state.deck.push_back({color, SKIP, -1});
            state.deck.push_back({color, REVERSE, -1});
            state.deck.push_back({color, DRAW_TWO, -1});
        }
    }
}

// draw one card from deck, reshuffle if needed
static InternalCard drawCard(GameState& state) {
    if (state.deck.empty()) {
        if (state.discardPile.size() > 1) {
            InternalCard topCard = state.discardPile.back();
            state.discardPile.pop_back();
            state.deck = state.discardPile;
            state.discardPile.clear();
            state.discardPile.push_back(topCard);
            mt19937 gen(1234);
            shuffle(state.deck.begin(), state.deck.end(), gen);
        }
    }
    if (state.deck.empty()) return {RED, NUMBER, 0};
    InternalCard card = state.deck.back();
    state.deck.pop_back();
    return card;
}

// check if a card can be played on top
static bool canPlay(const InternalCard& card, const InternalCard& topCard) {
    return card.color == topCard.color ||
           (card.type == NUMBER && topCard.type == NUMBER && card.value == topCard.value) ||
           (card.type == topCard.type && card.type != NUMBER);
}

// find which card can be played
static int findPlayableCard(const GameState& state, int player) {
    if (state.discardPile.empty()) return -1;
    if (player < 0 || player >= state.numPlayers) return -1;
    if (player >= (int)state.playerHands.size()) return -1;

    const InternalCard& topCard = state.discardPile.back();
    const auto& hand = state.playerHands[player];

    // try number match first for same color
    for (size_t i = 0; i < hand.size(); i++) {
        if (hand[i].color == topCard.color && hand[i].type == NUMBER) return i;
    }
    // then skip, reverse, draw two
    for (size_t i = 0; i < hand.size(); i++)
        if (hand[i].color == topCard.color && hand[i].type == SKIP) return i;
    for (size_t i = 0; i < hand.size(); i++)
        if (hand[i].color == topCard.color && hand[i].type == REVERSE) return i;
    for (size_t i = 0; i < hand.size(); i++)
        if (hand[i].color == topCard.color && hand[i].type == DRAW_TWO) return i;

    // same number or same action type
    if (topCard.type == NUMBER) {
        for (size_t i = 0; i < hand.size(); i++)
            if (hand[i].type == NUMBER && hand[i].value == topCard.value) return i;
    } else {
        for (size_t i = 0; i < hand.size(); i++)
            if (hand[i].type == topCard.type) return i;
    }
    return -1;
}

// move to next player
static void nextPlayer(GameState& state) {
    if (state.numPlayers <= 0) return;

    if (state.clockwise)
        state.currentPlayer = (state.currentPlayer + 1) % state.numPlayers;
    else
        state.currentPlayer = (state.currentPlayer - 1 + state.numPlayers) % state.numPlayers;

    if (state.currentPlayer < 0 || state.currentPlayer >= state.numPlayers)
        state.currentPlayer = 0;
}

UNOGame::UNOGame(int numPlayers) {
    if (numPlayers < 2) numPlayers = 2;
    if (numPlayers > 4) numPlayers = 4;

    GameState& state = gameStates[this];
    state.numPlayers = numPlayers;
    state.currentPlayer = 0;
    state.clockwise = true;
    state.gameOver = false;
    state.winner = -1;
    state.playerHands.resize(numPlayers);
}

// setup the game and shuffle
void UNOGame::initialize() {
    auto it = gameStates.find(this);
    if (it == gameStates.end()) return;
    GameState& state = it->second;

    if (state.numPlayers < 2 || state.numPlayers > 4) return;

    createDeck(state);
    if (state.deck.empty()) return;

    mt19937 gen(1234);
    shuffle(state.deck.begin(), state.deck.end(), gen);

    // deal 7 cards to each player
    for (int i = 0; i < 7; i++) {
        for (int p = 0; p < state.numPlayers; p++) {
            if (state.deck.empty()) break;
            state.playerHands[p].push_back(drawCard(state));
        }
    }

    if (!state.deck.empty()) state.discardPile.push_back(drawCard(state));

    state.currentPlayer = 0;
    state.clockwise = true;
    state.gameOver = false;
    state.winner = -1;
}

// handle one player's turn
void UNOGame::playTurn() {
    auto it = gameStates.find(this);
    if (it == gameStates.end()) return;

    GameState& state = it->second;
    if (state.gameOver) return;
    if (state.currentPlayer < 0 || state.currentPlayer >= state.numPlayers) return;
    if (state.discardPile.empty()) return;

    int cardIndex = findPlayableCard(state, state.currentPlayer);

    if (cardIndex != -1) {
        if (cardIndex < 0 || cardIndex >= (int)state.playerHands[state.currentPlayer].size()) {
            nextPlayer(state);
            return;
        }

        InternalCard playedCard = state.playerHands[state.currentPlayer][cardIndex];
        state.playerHands[state.currentPlayer].erase(state.playerHands[state.currentPlayer].begin() + cardIndex);
        state.discardPile.push_back(playedCard);

        // check winner
        if (state.playerHands[state.currentPlayer].empty()) {
            state.gameOver = true;
            state.winner = state.currentPlayer;
            return;
        }

        // handle action cards
        if (playedCard.type == SKIP) {
            nextPlayer(state);
        } else if (playedCard.type == REVERSE) {
            state.clockwise = !state.clockwise;
            if (state.numPlayers == 2) nextPlayer(state);
        } else if (playedCard.type == DRAW_TWO) {
            nextPlayer(state);
            if (state.currentPlayer >= 0 && state.currentPlayer < state.numPlayers) {
                state.playerHands[state.currentPlayer].push_back(drawCard(state));
                state.playerHands[state.currentPlayer].push_back(drawCard(state));
            }
        }
    } else {
        InternalCard drawnCard = drawCard(state);
        state.playerHands[state.currentPlayer].push_back(drawnCard);

        if (!state.discardPile.empty() && canPlay(drawnCard, state.discardPile.back())) {
            state.playerHands[state.currentPlayer].pop_back();
            state.discardPile.push_back(drawnCard);

            if (drawnCard.type == SKIP) {
                nextPlayer(state);
            } else if (drawnCard.type == REVERSE) {
                state.clockwise = !state.clockwise;
                if (state.numPlayers == 2) nextPlayer(state);
            } else if (drawnCard.type == DRAW_TWO) {
                nextPlayer(state);
                if (state.currentPlayer >= 0 && state.currentPlayer < state.numPlayers) {
                    state.playerHands[state.currentPlayer].push_back(drawCard(state));
                    state.playerHands[state.currentPlayer].push_back(drawCard(state));
                }
            }
        }
    }

    nextPlayer(state);
}

bool UNOGame::isGameOver() const {
    auto it = gameStates.find(this);
    if (it == gameStates.end()) return false;
    return it->second.gameOver;
}

int UNOGame::getWinner() const {
    auto it = gameStates.find(this);
    if (it == gameStates.end()) return -1;
    return it->second.winner;
}

// print basic state of game
string UNOGame::getState() const {
    auto it = gameStates.find(this);
    if (it == gameStates.end()) return "";

    const GameState& state = it->second;
    if (state.discardPile.empty()) return "";
    if (state.currentPlayer < 0 || state.currentPlayer >= state.numPlayers) return "";

    ostringstream oss;
    oss << "player " << state.currentPlayer << "'s turn, direction: "
        << (state.clockwise ? "clockwise" : "counter-clockwise")
        << ", top: " << state.discardPile.back().getColorString() << " "
        << state.discardPile.back().getValueString()
        << ", players cards: ";

    for (int i = 0; i < state.numPlayers; i++) {
        if (i < (int)state.playerHands.size())
            oss << "P" << i << ":" << state.playerHands[i].size();
        else
            oss << "P" << i << ":0";
        if (i < state.numPlayers - 1) oss << ", ";
    }
    return oss.str();
}