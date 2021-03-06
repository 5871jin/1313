import os
import pytest

from donor_models import Donor, Donor_Collection, get_sampleDB
import cli_main



def test_create_donor():
    """ Test to add a donor to the donor database"""
    donor = Donor("Violet Smith", 500)

    assert donor.name == "Violet Smith"


def test_add_donation():
    """ Test to add a donation amount to existing donor"""
    donor = Donor("Violet Smith", 500)

    donor.add_donation(100)

    assert donor.num_donations == 2
    assert donor.sum_donations == 600
    assert donor.last_donation == 100
    assert donor.average_donation == 300


def test_letter_generator():
    """ Test to generate a thank you letter for donor's last donation"""
    donor = Donor("Abby Lin", 5000)
    letter = donor.letter_generator(5000)

    print(letter)
    assert letter.startswith("\nDear Abby Lin,")
    assert letter.endswith("-The Team\n")
    assert "donation of $5000" in letter


def test_save_to_disk():
    """ Since we already tested the information contained in the thank you letter, 
    we only need to test if the letter is correctly saved to the disk"""
    donor = Donor("Abby Lin", 5000)
    donor.save_to_disk("thank_you_letters")

    os.chdir("thank_you_letters")
    assert os.path.isfile("Abby_Lin.txt")
    with open("Abby_Lin.txt") as f:
        size = len(f.read())
    assert size > 0


def test_report_generator():
    """ Test if the report for the sample donor database is accurate"""
    donor_Db = Donor_Collection(get_sampleDB())

    report = donor_Db.report_generator()

    assert report.startswith("Donor Name                 |  Total Given  | Num Gifts|  Average Gift |")
    assert "Christina Levermore        $       10000.00          1 $       10000.00" in report


def test_convert_to_std():
    """test to add a donor with improper capitalization of donor name"""
    name = "illy eLm"
    new_name = cli_main.conver_to_std(name)

    assert new_name == "Illy Elm"
    





# def test_donor_collection():
#     dc = DonorColletion()

#     dc.add_donor(Donor("Violet Smith"))

#     dc.find_donor()

#     dc.list_donors()

#     dc.creat_report()

