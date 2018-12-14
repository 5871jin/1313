#!/usr/bin/env python
import sys
import os


def get_sampleDB():
    """ Create a sample database for test """
    return [Donor("Fred Jones", [100, 500]),
            Donor("Christina Levermore", [10000]),
            Donor("Amy Lee", [9999, 500])]


class Donor_Collection():
    def __init__(self):
        self.donor_dict = {"Fred Jones": Donor("Fred Jones", 500)}
        self.sorted_dict = {"Fred Jones": Donor("Fred Jones", 500)}
        self.longest_name = "Fred Jones"
        self.largest_amount = 500


    def add_donor(self, donor_name, donation_amount):
        """
        Add a donor to the donor collection, and compare the length of the donor's name and donation amount to decide the widths we are going to use in generating reports

        :param: name of the donor and donation amount

        :return: the new donor database
        """
        if donor_name in self.donor_dict:
            self.donor_dict[donor_name].add_donation(donation_amount)
        else:
            self.donor_dict[donor_name]=Donor(donor_name, donation_amount)
        if len(donor_name) > len(self.longest_name):
            self.longest_name = donor_name
        if self.donor_dict[donor_name].sum_donations > self.largest_amount:
            self.largest_amount = self.donor_dict[donor_name].sum_donations
        self.sort_report()


    def sort_report(self):
        self.sorted_dict = sorted(self.donor_dict.items(), key=lambda x: x[1].sum_donations,reverse=True)


    def send_single(self, donor, last_donation_amount):
        return self.donor_dict[donor].letter_generator(last_donation_amount)


    def report_generator(self):
        """
        Generate a report of the donors donation history

        :return: a string type report
        """
        report =""
        width = len(self.longest_name) + 8
        width2 = len(str(self.largest_amount)) + 8
        report += "{:{width}}{}{:^{width2}}{}{:^10}{}{:^{width2}}{}\n".format("      Donor Name","|"," Total Given","|"," Num Gifts","|",         
                  "Average Gift","|",width=width,width2=width2)
        report += "-"*(width + width2*2 +16) + "\n"
        for x in self.sorted_dict:
            report += "{:{width}}{:1}{:{width2}.2f}{:>11}{:>2}{:>{width2}.2f}\n".format(x[0],"$", x[1].sum_donations,x[1].num_donations," $",round(x[1].average_donation,2),width=width,width2=width2)
            
        return report


    def save_all(self, out_path):
        [donor.save_to_disk(out_path) for donor in self.donor_dict.values()]


    def list_donors(self):
        """
        Create a list of existing donors and return a string
        """
        list = ["Donor List:"]
        pass


class Donor():
    def __init__(self, name, donation):
        self.name = name
        self.donations = []
        self.donations.append(donation)


    def add_donation(self, donation):
        self.donations.append(donation)


    def save_to_disk(self, out_path):
        """
        save the thank you letters to the disk

        :param: the letters out path
        :return: return a txt file in the working directory
        """
        ty_letter = self.letter_generator(self.sum_donations)
        filename = self.name.replace(" ","_") + ".txt"
        print("writing to " + self.name + "......\n")
        filename = os.path.join(out_path, filename)
        open(filename, "w").write(ty_letter)


    def letter_generator(self, donation_amount):
        """
        Generate a thank you letter for donors

        :param: donation amount
        :return: a string type letter which stores as a txt file in the working directory
        """
        letter = "\nDear " + self.name + ",\n" + "\tThank you for your very kind donation of $" + str(donation_amount) 
        letter += ".\n" + "\tIt will be put to very good use.\n \t\tSincerely,\n \t\t-The Team\n"
        return letter


    @property
    def num_donations(self):
        return len(self.donations)


    @property
    def last_donation(self):
        return self.donations[-1]


    @property
    def sum_donations(self):
        return sum(self.donations)


    @property
    def average_donation(self):
        return self.sum_donations / self.num_donations
    


   