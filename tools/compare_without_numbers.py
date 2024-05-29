import re
import sys

def normalize_line(line):
    line = re.sub(r'(?<!\')\b\d+(\.\d+)?\b(?!\')', 'NUMBER', line)
    line = re.sub(r'\'\d+([.,]\d+)*\'', '\'NUMBER\'', line)
    return line

def compare_files(file1_path, file2_path):
    try:
        with open(file1_path, 'r') as file1, open(file2_path, 'r') as file2:
            file1_lines = file1.readlines()
            file2_lines = file2.readlines()
            
            if len(file1_lines) != len(file2_lines):
                print("Files have different number of lines.")
                return 1
            
            for i, (line1, line2) in enumerate(zip(file1_lines, file2_lines), start=1):
                line1 = line1.strip()
                line2 = line2.strip()
                
                normalized_line1 = normalize_line(line1)
                normalized_line2 = normalize_line(line2)
                
                if normalized_line1 != normalized_line2:
                    print(f"Lines {i} differ:\n{file1_path}: '{line1}'\n{file2_path}: '{line2}'")
                    return 1
                    
        return 0
    except Exception as e:
        print(f"An error occurred: {e}")
        return 1

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compare_files.py <file1> <file2>")
        sys.exit(1)
    
    file1_path = sys.argv[1]
    file2_path = sys.argv[2]
    
    result = compare_files(file1_path, file2_path)
    sys.exit(result)
