// uno.cpp
#include "uno.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <map>

enum Color { RED, GREEN, BLUE, YELLOW };
enum Type { NUMBER, SKIP, REVERSE, DRAW_TWO };

struct InternalCard {
    Color color;
    Type type;
    int value; // 0-9 for NUMBER, -1 for action cards

    std::string getColorString() const {
        switch(color) {
            case RED: return "Red";
            case GREEN: return "Green";
            case BLUE: return "Blue";
            case YELLOW: return "Yellow";
        }
        return "";
    }

    std::string getValueString() const {
        if (type == NUMBER) return std::to_string(value);
        if (type == SKIP) return "Skip";
        if (type == REVERSE) return "Reverse";
        if (type == DRAW_TWO) return "Draw Two";
        return "";
    }
};

struct GameState {
    int numPlayers;
    std::vector<std::vector<InternalCard>> playerHands;
    std::vector<InternalCard> deck;
    std::vector<InternalCard> discardPile;
    int currentPlayer;
    bool clockwise;
    bool gameOver;
    int winner;
};

static std::map<const UNOGame*, GameState> gameStates;

static void createDeck(GameState& state) {
    state.deck.clear();
    state.deck.reserve(76); // Pre-allocate for efficiency

    // For each color
    for (int c = 0; c < 4; c++) {
        Color color = static_cast<Color>(c);

        // One 0 card per color
        state.deck.push_back({color, NUMBER, 0});

        // Two of each 1-9 per color
        for (int v = 1; v <= 9; v++) {
            state.deck.push_back({color, NUMBER, v});
            state.deck.push_back({color, NUMBER, v});
        }

        // Two Skip cards per color
        state.deck.push_back({color, SKIP, -1});
        state.deck.push_back({color, SKIP, -1});

        // Two Reverse cards per color
        state.deck.push_back({color, REVERSE, -1});
        state.deck.push_back({color, REVERSE, -1});

        // Two Draw Two cards per color
        state.deck.push_back({color, DRAW_TWO, -1});
        state.deck.push_back({color, DRAW_TWO, -1});
    }
}

static InternalCard drawCard(GameState& state) {
    if (state.deck.empty()) {
        // Reshuffle discard pile into deck (keep top card)
        if (state.discardPile.size() > 1) {
            InternalCard topCard = state.discardPile.back();
            state.discardPile.pop_back();
            state.deck = state.discardPile;
            state.discardPile.clear();
            state.discardPile.push_back(topCard);
            std::mt19937 gen(1234);
            std::shuffle(state.deck.begin(), state.deck.end(), gen);
        }
    }
    // If still empty after reshuffle attempt, return dummy card (stalemate scenario)
    if (state.deck.empty()) {
        return {RED, NUMBER, 0};
    }
    InternalCard card = state.deck.back();
    state.deck.pop_back();
    return card;
}

static bool canPlay(const InternalCard& card, const InternalCard& topCard) {
    return card.color == topCard.color ||
           (card.type == NUMBER && topCard.type == NUMBER && card.value == topCard.value) ||
           (card.type == topCard.type && card.type != NUMBER);
}

static int findPlayableCard(const GameState& state, int player) {
    // Safety checks
    if (state.discardPile.empty()) return -1;
    if (player < 0 || player >= state.numPlayers) return -1;
    if (player >= (int)state.playerHands.size()) return -1;

    const InternalCard& topCard = state.discardPile.back();
    const auto& hand = state.playerHands[player];

    // Priority: color match (number first, then Skip, Reverse, Draw Two), then value match, then action match
    // First, find color matches - numbers first
    for (size_t i = 0; i < hand.size(); i++) {
        if (hand[i].color == topCard.color && hand[i].type == NUMBER) {
            return i;
        }
    }
    // Color match - Skip
    for (size_t i = 0; i < hand.size(); i++) {
        if (hand[i].color == topCard.color && hand[i].type == SKIP) {
            return i;
        }
    }
    // Color match - Reverse
    for (size_t i = 0; i < hand.size(); i++) {
        if (hand[i].color == topCard.color && hand[i].type == REVERSE) {
            return i;
        }
    }
    // Color match - Draw Two
    for (size_t i = 0; i < hand.size(); i++) {
        if (hand[i].color == topCard.color && hand[i].type == DRAW_TWO) {
            return i;
        }
    }

    // Value match for numbers
    if (topCard.type == NUMBER) {
        for (size_t i = 0; i < hand.size(); i++) {
            if (hand[i].type == NUMBER && hand[i].value == topCard.value) {
                return i;
            }
        }
    }

    // Action match (same action type)
    if (topCard.type != NUMBER) {
        for (size_t i = 0; i < hand.size(); i++) {
            if (hand[i].type == topCard.type) {
                return i;
            }
        }
    }

    return -1; // No playable card
}

static void nextPlayer(GameState& state) {
    // Safety check
    if (state.numPlayers <= 0) return;

    if (state.clockwise) {
        state.currentPlayer = (state.currentPlayer + 1) % state.numPlayers;
    } else {
        state.currentPlayer = (state.currentPlayer - 1 + state.numPlayers) % state.numPlayers;
    }

    // Extra safety: ensure valid player index
    if (state.currentPlayer < 0 || state.currentPlayer >= state.numPlayers) {
        state.currentPlayer = 0;
    }
}

UNOGame::UNOGame(int numPlayers) {
    // Clamp numPlayers to valid range
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

void UNOGame::initialize() {
    auto it = gameStates.find(this);
    if (it == gameStates.end()) return; // Not constructed properly

    GameState& state = it->second;

    // Validate number of players
    if (state.numPlayers < 2 || state.numPlayers > 4) {
        return; // Invalid player count
    }

    createDeck(state);

    // Safety check: ensure deck was created
    if (state.deck.empty()) {
        return;
    }

    // Shuffle with fixed seed
    std::mt19937 gen(1234);
    std::shuffle(state.deck.begin(), state.deck.end(), gen);

    // Deal 7 cards to each player
    for (int i = 0; i < 7; i++) {
        for (int p = 0; p < state.numPlayers; p++) {
            if (state.deck.empty()) break; // Shouldn't happen but safety check
            state.playerHands[p].push_back(drawCard(state));
        }
    }

    // First card to discard pile
    if (!state.deck.empty()) {
        state.discardPile.push_back(drawCard(state));
    }

    state.currentPlayer = 0;
    state.clockwise = true;
    state.gameOver = false;
    state.winner = -1;
}

void UNOGame::playTurn() {
    auto it = gameStates.find(this);
    if (it == gameStates.end()) return; // Game not initialized

    GameState& state = it->second;

    if (state.gameOver) return;

    // Safety check: validate current player
    if (state.currentPlayer < 0 || state.currentPlayer >= state.numPlayers) {
        return;
    }

    // Safety check: ensure discard pile has at least one card
    if (state.discardPile.empty()) {
        return;
    }

    int cardIndex = findPlayableCard(state, state.currentPlayer);

    if (cardIndex != -1) {
        // Safety check: validate card index
        if (cardIndex < 0 || cardIndex >= (int)state.playerHands[state.currentPlayer].size()) {
            nextPlayer(state);
            return;
        }

        // Play the card
        InternalCard playedCard = state.playerHands[state.currentPlayer][cardIndex];
        state.playerHands[state.currentPlayer].erase(state.playerHands[state.currentPlayer].begin() + cardIndex);
        state.discardPile.push_back(playedCard);

        // Check for winner
        if (state.playerHands[state.currentPlayer].empty()) {
            state.gameOver = true;
            state.winner = state.currentPlayer;
            return;
        }

        // Handle action cards
        if (playedCard.type == SKIP) {
            nextPlayer(state); // Skip next player
        } else if (playedCard.type == REVERSE) {
            state.clockwise = !state.clockwise;
            // In 2-player game, reverse is effectively a skip
            if (state.numPlayers == 2) {
                nextPlayer(state);
            }
        } else if (playedCard.type == DRAW_TWO) {
            nextPlayer(state);
            // Validate player after advance
            if (state.currentPlayer >= 0 && state.currentPlayer < state.numPlayers) {
                // Next player draws 2 cards
                state.playerHands[state.currentPlayer].push_back(drawCard(state));
                state.playerHands[state.currentPlayer].push_back(drawCard(state));
            }
            // Skip happens implicitly (don't advance again)
        }
    } else {
        // Draw a card
        InternalCard drawnCard = drawCard(state);
        state.playerHands[state.currentPlayer].push_back(drawnCard);

        // Check if drawn card can be played (only if discard pile not empty)
        if (!state.discardPile.empty() && canPlay(drawnCard, state.discardPile.back())) {
            // Play it immediately
            state.playerHands[state.currentPlayer].pop_back();
            state.discardPile.push_back(drawnCard);

            // Handle action cards
            if (drawnCard.type == SKIP) {
                nextPlayer(state);
            } else if (drawnCard.type == REVERSE) {
                state.clockwise = !state.clockwise;
                if (state.numPlayers == 2) {
                    nextPlayer(state);
                }
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

std::string UNOGame::getState() const {
    auto it = gameStates.find(this);
    if (it == gameStates.end()) return ""; // Not initialized

    const GameState& state = it->second;

    // Safety checks
    if (state.discardPile.empty()) return "";
    if (state.currentPlayer < 0 || state.currentPlayer >= state.numPlayers) return "";

    std::ostringstream oss;
    oss << "Player " << state.currentPlayer << "'s turn, Direction: ";
    oss << (state.clockwise ? "Clockwise" : "Counter-clockwise");
    oss << ", Top: " << state.discardPile.back().getColorString() << " "
        << state.discardPile.back().getValueString();
    oss << ", Players cards: ";
    for (int i = 0; i < state.numPlayers; i++) {
        if (i < (int)state.playerHands.size()) {
            oss << "P" << i << ":" << state.playerHands[i].size();
        } else {
            oss << "P" << i << ":0"; // Safety fallback
        }
        if (i < state.numPlayers - 1) oss << ", ";
    }
    return oss.str();
}