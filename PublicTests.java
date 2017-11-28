package cmsc131;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Random;
import java.util.stream.Collectors;

import org.junit.Test;

import cmsc131.cards.Card;
import cmsc131.cards.Rank;
import cmsc131.cards.Suit;
import cmsc131.poker.CardComparator;
import cmsc131.poker.MyPokerAnalysis;
import cmsc131.poker.PokerHand;

public class PublicTests {

  @Test
  public void testPokerComparatorSuitsIgnored() {
    CardComparator SUITS_IGNORED = new CardComparator(false);
    assertFalse(SUITS_IGNORED.isSuitMatters());
    Card FIVE_OF_HEARTS = new Card(Rank.FIVE, Suit.HEARTS);
    Card THREE_OF_HEARTS = new Card(Rank.THREE, Suit.HEARTS);
    Card FIVE_OF_CLUBS = new Card(Rank.FIVE, Suit.CLUBS);
    Card QUEEN_OF_DIAMONDS = new Card(Rank.QUEEN, Suit.DIAMONDS);
    assertTrue(SUITS_IGNORED.compare(THREE_OF_HEARTS, FIVE_OF_HEARTS) < 0);
    assertTrue(SUITS_IGNORED.compare(FIVE_OF_HEARTS, THREE_OF_HEARTS) > 0);
    assertTrue(SUITS_IGNORED.compare(FIVE_OF_HEARTS, QUEEN_OF_DIAMONDS) < 0);
    assertTrue(SUITS_IGNORED.compare(FIVE_OF_CLUBS, FIVE_OF_HEARTS) == 0);
  }

  @Test
  public void testPokerComparatorSuitsMatter() {
    CardComparator SUITS_MATTER = new CardComparator(true);
    assertTrue(SUITS_MATTER.isSuitMatters());
    Card FIVE_OF_HEARTS = new Card(Rank.FIVE, Suit.HEARTS);
    Card THREE_OF_HEARTS = new Card(Rank.THREE, Suit.HEARTS);
    Card FIVE_OF_CLUBS = new Card(Rank.FIVE, Suit.CLUBS);
    Card QUEEN_OF_DIAMONDS = new Card(Rank.QUEEN, Suit.DIAMONDS);
    assertTrue(SUITS_MATTER.compare(THREE_OF_HEARTS, THREE_OF_HEARTS) == 0);
    assertTrue(SUITS_MATTER.compare(THREE_OF_HEARTS, FIVE_OF_HEARTS) < 0);
    assertTrue(SUITS_MATTER.compare(FIVE_OF_HEARTS, THREE_OF_HEARTS) > 0);
    assertTrue(SUITS_MATTER.compare(FIVE_OF_CLUBS, FIVE_OF_HEARTS) < 0);
    assertTrue(SUITS_MATTER.compare(FIVE_OF_HEARTS, FIVE_OF_CLUBS) > 0);
  }

  @Test
  public void testHighCard() {
    test("3D,4C,6H,2C,AH", PokerHand.HIGH_CARD);
    test("3D,4C,6H,7C,KD", PokerHand.HIGH_CARD);

  }

  @Test
  public void testPair() {
    test("3D,4C,6H,3C,AH", PokerHand.PAIR);
    test("3D,4C,6H,7C,6D", PokerHand.PAIR);
  }

  @Test
  public void testPairSevenCards() {
    test("3D,4C,6H,3C,AH,TC,QC", PokerHand.PAIR);
    test("3D,4C,6H,7C,6D,TC,QC", PokerHand.PAIR);
  }

  @Test
  public void testTwoPair() {
    test("3D,4C,6H,3C,4H", PokerHand.TWO_PAIR);
    test("3D,3C,6H,7C,6D", PokerHand.TWO_PAIR);
  }

  @Test
  public void testTwoPairSevenCards() {
    test("3D,4C,6H,3C,4H,TC,QC", PokerHand.TWO_PAIR);
    test("3D,3C,6H,7C,6D,TC,QC", PokerHand.TWO_PAIR);
  }

  @Test
  public void testThreeOfAKind() {
    test("3D,3S,6H,3C,4H", PokerHand.THREE_OF_A_KIND);
    test("6S,3C,6H,7C,6D", PokerHand.THREE_OF_A_KIND);
  }

  @Test
  public void testFullHouse() {
    test("3D,3S,9S,9C,3H", PokerHand.FULL_HOUSE);
    test("6S,QC,6H,QS,6D", PokerHand.FULL_HOUSE);
  }

  @Test
  public void testFourOfAKind() {
    test("3D,3S,3H,3C,4H", PokerHand.FOUR_OF_A_KIND);
    test("6S,6C,6H,7C,6D", PokerHand.FOUR_OF_A_KIND);
  }

  @Test
  public void testFlush() {
    test("3D,5D,2D,KD,TD", PokerHand.FLUSH);
    test("9C,7C,AC,4C,5C", PokerHand.FLUSH);
  }

  @Test
  public void testStraight() {
    Random r = new Random(55);
    test("2C,3S,4D,5D,6H", r, PokerHand.STRAIGHT);
    test("9H,TS,JS,QC,KS", r, PokerHand.STRAIGHT);
  }

  @Test
  public void testStraightFlush() {
    Random r = new Random(59);
    test("2D,3D,4D,5D,6D", r, PokerHand.STRAIGHT_FLUSH);
    test("9S,TS,JS,QS,KS", r, PokerHand.STRAIGHT_FLUSH);
  }

  @Test
  public void testBlackjack() {
    test("29", 11);
    test("35", 8);
    test("357", 15);
  }

  @Test
  public void testBlackjackFacecard() {
    test("2Q", 12);
    test("7K", 17);
    test("JQ", 20);
  }

  @Test
  public void testBlackjackAce() {
    test("2A", 13);
    test("5A9", 15);
    test("AAA5", 18);
  }

  static ArrayList<Card> parse(String hand) {
    List<String> cards = Arrays.asList(hand.split(","));
    return cards.stream().map(PublicTests::parseShortName).collect(Collectors.toCollection(ArrayList::new));
  }

  static Suit randomSuit(Random r) {
    Suit[] v = Suit.values();
    return v[r.nextInt(v.length)];
  }

  static List<Card> parseBlackjack(String ranks) {
    List<Card> cards = new ArrayList<>(ranks.length());
    Random r = new Random(ranks.hashCode());
    for (int i = 0; i < ranks.length(); i++) {
      Rank rank = parseRank(ranks.charAt(i));
      cards.add(new Card(rank, randomSuit(r)));
    }
    Collections.shuffle(cards, r);
    return cards;
  }

  static Card parseShortName(String shortName) {
    shortName = shortName.trim().toUpperCase();
    if (shortName.length() != 2)
      throw new IllegalArgumentException(shortName);
    Rank rank = parseRank(shortName.charAt(0));
    Suit suit = Suit.values()["CDHS".indexOf(shortName.charAt(1))];
    return new Card(rank, suit);
  }

  private static Rank parseRank(char c) {
    return Rank.values()["23456789TJQKA".indexOf(c)];
  }

  static void test(String hand, PokerHand expected) {
    test(hand, null, expected);
  }

  static void test(String hand, Random shuffle, PokerHand expected) {
    ArrayList<Card> parse = parse(hand);
    if (shuffle != null)
      Collections.shuffle(parse, shuffle);
    MyPokerAnalysis analyze = new MyPokerAnalysis(parse);
    if (!expected.matches(analyze))
      fail("For " + hand + " expected " + expected + " but hasXXX method returned false");
    PokerHand actual = PokerHand.get(analyze);
    assertEquals("For " + hand, expected, actual);
  }

  static void test(String ranks, int expected) {
    List<Card> parse = parseBlackjack(ranks);
    cmsc131.blackjack.BlackjackAnalyzer analyze = new cmsc131.blackjack.BlackjackAnalyzer();
    boolean hasAce = false;
    for (Card c : parse) {
      analyze.takeCard(c);
      if (c.getRank() == Rank.ACE)
        hasAce = true;
    }
    assertEquals(hasAce, analyze.hasAce());
    assertEquals("For " + ranks, expected, analyze.getValue());
  }

}
