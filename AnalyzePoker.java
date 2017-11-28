package cmsc131.poker;

/**
 * This interface is implemented by classes that can be used to analyze a poker
 * hand to determine what poker hands it contains.
 * 
 * Note: You only need to worry about the highest poker hand a set of cards
 * contains. For example, if a set of cards contains four of a kind, it doesn't
 * matter what your analysis returns if asked if the cards contain three of a
 * kind or two pair: It won't be tested.
 * 
 * If you aren't to the point of implementing a method, just return false.
 * 
 * Note that you might be given more than 5 cards. For example, you might be
 * given 7 cards. You need to determine if there exist 5 cards that satisfy the
 * requirment of each hand.
 * 
 * Poker hands are described at
 * https://en.wikipedia.org/wiki/List_of_poker_hands
 *
 */
public interface AnalyzePoker {

  /** has two cards with the same rank */
  boolean hasPair();

  /** has three cards with the same rank */
  boolean hasThreeOfAKind();

  /**
   * there are two different ranks such that there are two cards of each of
   * those ranks
   */
  boolean hasTwoPair();

  /** has four cards with the same rank */
  boolean hasFourOfAKind();

  /** Has both a triple, and also a pair of a different rank */
  boolean hasFullHouse();

  /*
   * You can get most of the posts for the project without providing an
   * implementation for these methods.
   */

  /**
   * Has 5 cards with consecutive ranks. Note that an Ace can be counted as low
   * or high for a straight.
   */
  boolean hasStraight();

  /** Has 5 cards of the same suite */
  boolean hasFlush();

  /** Has 5 cards with consecutive ranks all of the same suit. */
  boolean hasStraightFlush();

}