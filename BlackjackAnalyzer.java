package cmsc131.blackjack;

import cmsc131.cards.Card;

public class BlackjackAnalyzer {

  /** Construct a new Blackjack analyzer */
  public BlackjackAnalyzer() {
    throw new UnsupportedOperationException("Replace with your implementation");

  }

  /**
   * Do the cards seen so far include an Ace?
   */
  public boolean hasAce() {
    throw new UnsupportedOperationException("Replace with your implementation");

  }

  /**
   * Return the current value of the cards seen so far. An ace can count as
   * either 1 or 11, face cards are worth 10, and other cards are worth their
   * rank (e.g., a FIVE is worth 5).
   * 
   * If the cards seen contains an Ace, the value could be one of several
   * different values. Return the highest value that is less than or equal to
   * 21.
   * 
   * If the value is greater than 21, the cards have gone bust, return -1.
   */

  public int getValue() {
    throw new UnsupportedOperationException("Replace with your implementation");

  }

  /**
   * Take one card, update any internal state needed to be able to answer
   * queries.
   */
  public void takeCard(Card c) {
    throw new UnsupportedOperationException("Replace with your implementation");
  }

}
